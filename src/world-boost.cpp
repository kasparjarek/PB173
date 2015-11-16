#include <iostream>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fstream>

#include "world.h"

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
    cout << "Usage:" << endl;

    cout << "\t" << "--area-size <N> <M>" << endl;
    cout << "\t\t" << "game area will have size <N> x <M>" << endl;

    cout << "\t" << "--green-tanks <N>" << endl;
    cout << "\t\t" << "creates <N> green tanks" << endl;

    cout << "\t" << "--red-tanks <N>" << endl;
    cout << "\t\t" << "creates <N> red tanks" << endl;

    cout << "\t" << "-d, --daemonize" << endl;
    cout << "\t\t" << "run world as daemon" << endl;

    cout << "\t" << "-p, --pipe <path>" << endl;
    cout << "\t\t" << "use <path> as a FIFO pipe for worldclient program" << endl;

    cout << "\t" << "--round-time <N>" << endl;
    cout << "\t\t" << "set duration of one round to be <N> seconds" << endl;

    cout << "\t" << "-h, --help" << endl;
    cout << "\t\t" << "shows this help" << endl << endl;
}

/* Signal handling */

volatile bool done = false;
volatile bool restart = false;

static void sigHandler(int signo)
{
    if (signo == SIGQUIT || signo == SIGTERM || signo == SIGINT) {
        done = true;
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

    if (sigaction(SIGQUIT, &sigAction, NULL) != 0) {
        syslog(LOG_ERR, "sigaction() failed: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGINT, &sigAction, NULL) != 0) {
        syslog(LOG_ERR, "sigaction() failed: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGTERM, &sigAction, NULL) != 0) {
        syslog(LOG_ERR, "sigaction() failed: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGUSR1, &sigAction, NULL) != 0) {
        syslog(LOG_ERR, "sigaction() failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

/* Option parsing */

struct worldOptions {
    int	areaX;
    int areaY;
    int greenCount;
    int redCount;
    bool daemonize;
    std::string pipePath;
    useconds_t roundTime;
};

bool checkOptions(struct worldOptions & options)
{
    // TODO: check if options are valid
    return true;
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
        cout << "Some required options were not provided" << endl;
        printHelp();
        exit(1);
    }

    return checkOptions(options);
}

/* Main */

int main(int argc, char *argv[])
{
    /* Parse options */

    struct worldOptions options;
    if (!parseOptions(argc, argv, options)) {
        cout << "invalid options provided" << endl;
        exit(1);
    }

    /* Check if there is another instance of world running */

    char const *worldPidPath = "/var/run/world.pid";
    FILE *worldPid;
    if ((worldPid = fopen(worldPidPath, "a+")) == NULL) {
        worldPidPath = "world.pid";
        if ((worldPid = fopen(worldPidPath, "a+")) == NULL) {
            syslog(LOG_ERR, "cannot open %s: %s", worldPidPath, strerror(errno));
            exit(1);
        }
    }

    pid_t savedPid;
    if (fscanf(worldPid, "%d", &savedPid) == EOF) {
        fprintf(worldPid, "%d", getpid());
        fclose(worldPid);
    } else {
        if (savedPid != getpid()) {
            syslog(LOG_INFO, "world pid %d is already running", savedPid);
            fclose(worldPid);
            exit(0);
        }
    }

    /* Daemonize */

    if (options.daemonize) {
        if (daemon(1, 0) != 0) {
            syslog(LOG_ERR, "daemon() failed: %s", strerror(errno));
            unlink(worldPidPath); //<< should go to try-catch block
            exit(1);
        }
        openlog(NULL, 0, LOG_DAEMON);
    }

    /* Create Pipe if doesn't exist */

    if (access(options.pipePath.c_str(), F_OK) != 0) {
        if (mkfifo(options.pipePath.c_str(), S_IRUSR | S_IWUSR) != 0) {
            syslog(LOG_ERR, "mkfifo() with name %s failed: %s",options.pipePath.c_str(), strerror(errno));
            unlink(worldPidPath);
            return 1;
        }
    }

    /* Set signal actions */

    if (setSigHandler() != 0) {
        unlink(worldPidPath);
        unlink(options.pipePath.c_str());
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
        syslog(LOG_ERR, "World throw expection: %s", error.what());

        unlink(options.pipePath.c_str());
        unlink(worldPidPath);
        return -1;
    }


    unlink(options.pipePath.c_str());
    unlink(worldPidPath);

    return 0;
}

