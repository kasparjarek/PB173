#include "world.h"

#include <fcntl.h>
#include <getopt.h>
#include <libintl.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <fstream>
#include <iostream>
#include <sys/inotify.h>

#define _(STRING) gettext(STRING)

using std::cout;
using std::endl;


const char *ARGS = "hdp:";
const struct option LONG_ARGS[] = {
    {"help", no_argument, NULL, 'h'},
    {"area-size", required_argument, NULL, 0},
    {"green-tanks", required_argument, NULL, 0},
    {"red-tanks", required_argument, NULL, 0},
    {"daemonize", no_argument, NULL, 'd'},
    {"pipe", required_argument, NULL, 'p'},
    {"round-time", required_argument, NULL, 0},
    {0, 0, 0, 0}
};

void printHelp()
{
    cout << _("Usage:") << endl;

    cout << "\t" << "--area-size <N> <M>" << endl;
    cout << "\t\t" << _("game area will have size <N> x <M>") << endl;

    cout << "\t" << "--green-tanks <N>" << endl;
    cout << "\t\t" << _("creates <N> green tanks") << endl;

    cout << "\t" << "--red-tanks <N>" << endl;
    cout << "\t\t" << _("creates <N> red tanks") << endl;

    cout << "\t" << "-d, --daemonize" << endl;
    cout << "\t\t" << _("run world as daemon") << endl;

    cout << "\t" << "-p, --pipe <path>" << endl;
    cout << "\t\t" << _("use <path> as a FIFO pipe for worldclient program") << endl;

    cout << "\t" << "--round-time <N>" << endl;
    cout << "\t\t" << _("set duration of one round to be <N> microseconds") << endl;

    cout << "\t" << "-h, --help" << endl;
    cout << "\t\t" << _("shows this help") << endl << endl;
}

/* Signal handling */

volatile bool done = false;
volatile bool restart = false;

static void sigHandler(int signo)
{
    if (signo == SIGQUIT || signo == SIGTERM || signo == SIGINT) {
        done = true;
    }
    else if (signo == SIGPIPE) {
        done = true;
        syslog(LOG_WARNING, "Catch SIGPIPE, terminating");
    }
    else if (signo == SIGUSR1) {
        restart = true;
    }
    else {
        syslog(LOG_ERR, "Error: invalid signal");
    }
}

int setSigHandler()
{
    struct sigaction sigAction;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigAction.sa_handler = sigHandler;

    if (sigaction(SIGQUIT, &sigAction, NULL) != 0 ||
        sigaction(SIGINT, &sigAction, NULL) != 0 ||
        sigaction(SIGTERM, &sigAction, NULL) != 0 ||
        sigaction(SIGUSR1, &sigAction, NULL) != 0 ||
        sigaction(SIGPIPE, &sigAction, NULL) != 0) {

        syslog(LOG_ERR, "sigaction() failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/* Option parsing */

struct worldOptions {
    int	areaX = 0;
    int areaY = 0;
    int greenCount = 0;
    int redCount = 0;
    bool daemonize = 0;
    std::string pipePath;
    useconds_t roundTime = 0;
};

bool checkOptions(struct worldOptions & options)
{
    return !(options.areaX <= 0 || options.areaY <= 0 ||
            options.redCount < 0 || options.greenCount < 0 ||
            (options.areaY * options.areaX <= options.redCount + options.greenCount));
}

bool parseOptions(int argc, char **argv, struct worldOptions & options)
{
    int opt;
    int optindex;

    bool area = false;
    bool gcnt = false;
    bool rcnt = false;
    bool rndt = false;
    bool ppth = false;

    while ((opt = getopt_long(argc, argv, ARGS, LONG_ARGS, &optindex)) != -1) {
        switch (opt) {
        case 0: // longopts only
            switch (optindex) {
            case 1: // --area-size
                options.areaX = atoi(argv[optind-1]);
                options.areaY = atoi(argv[optind++]);
                area = true;
                break;
            case 2: // --green-tanks
                options.greenCount = atoi(optarg);
                gcnt = true;
                break;
            case 3: // --red-tanks
                options.redCount = atoi(optarg);
                rcnt = true;
                break;
            case 6: // --round-time
                options.roundTime = atoi(optarg);
                rndt = true;
            default:
                break;
            }
            break;

        case 'h':   // --help
            printHelp();
            exit(0);
        case 'd':   // --daemonize
            options.daemonize = true;
            break;
        case 'p':   // --pipe
            options.pipePath = optarg;
            ppth = true;
            break;
        default:
            printHelp();
            exit(1);
        }
    }

    if (!area || !gcnt || !rcnt || !rndt || !ppth) {
        cout << _("Some required options were not provided") << endl;
        printHelp();
        exit(1);
    }

    return checkOptions(options);
}

void closePidFile(const char * path, int fd){
    flock(fd, LOCK_UN);
    truncate(path, 0);
    sleep(1);
    if(flock(fd, LOCK_EX) != -1){
        unlink(path);
    }
}

/* Main */

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    bindtextdomain("world", "../locale");
    textdomain("world");

    /* Parse options */

    struct worldOptions options;
    if (!parseOptions(argc, argv, options)) {
        cout << _("invalid options provided") << endl;
        exit(1);
    }

    /* Check if there is another instance of world running */

    const char *worldPidPath = "world.pid";
    int worldFD = open(worldPidPath, O_WRONLY | O_CREAT, 0777);

    while(flock(worldFD, LOCK_EX) == -1){
        syslog(LOG_INFO, "world.pid already locked");
        int inot = inotify_init();
        if(inot == -1){
            cout << "failed inotify_init" << endl;
            syslog(LOG_ERR, "failed inotify_init");
            close(worldFD);
            return 1;
        }
        int ret_val = inotify_add_watch(inot, worldPidPath, IN_DELETE);
        if(ret_val == -1){
            syslog(LOG_ERR, "failed inotify_add_watch");
            cout << "failed inot_add_watch" << endl;
            close(inot);
            close(worldFD);
            return 1;
        }
        select(inot, NULL, NULL, NULL, NULL);
    }

    syslog(LOG_INFO, "world.pid is mine now");
    cout << "lock acquired" << endl;

    std::string pid = std::to_string(getpid());
    if(write(worldFD, pid.c_str(), pid.size()) == -1){
        cout << "couldn't write pid" << endl;
        syslog(LOG_ERR, "couldn't write pid");
    }


    /* Daemonize */

    if (options.daemonize) {
        if (daemon(1, 0) != 0) {
            syslog(LOG_ERR, "daemon() failed: %s", strerror(errno));
            closePidFile(worldPidPath, worldFD);
            exit(1);
        }
        openlog(NULL, 0, LOG_DAEMON);
    }

    /* Create Pipe if doesn't exist */

    if (access(options.pipePath.c_str(), F_OK) != 0) {
        if (mkfifo(options.pipePath.c_str(), S_IRUSR | S_IWUSR) != 0) {
            syslog(LOG_ERR, "mkfifo() with name %s failed: %s",options.pipePath.c_str(), strerror(errno));
            closePidFile(worldPidPath, worldFD);
            return 1;
        }
    }

    /* Set signal actions */

    if (setSigHandler() != 0) {
        closePidFile(worldPidPath, worldFD);
        return -1;
    }

    /* Run game */

    try {
        World world(options.areaX, options.areaY, options.redCount,
                    options.greenCount, options.pipePath, options.roundTime);

        world.init();

        while (!done) {
            if (restart) {
                world.init();
                restart = false;
            } else {
                world.performRound();
            }
        }
    } catch(std::runtime_error error) {
        syslog(LOG_ERR, "World threw expection: %s", error.what());

        closePidFile(worldPidPath, worldFD);
        return -1;
    }

    closePidFile(worldPidPath, worldFD);
    return 0;
}

