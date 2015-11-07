#include <iostream>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "world.h"

using namespace std;

const char *ARGS = "hdp:";
const struct option LONG_ARGS[] = {
    {"help", no_argument, NULL, 'h'},
    {"area-size", required_argument, NULL, 0},
    {"green-tanks", required_argument, NULL, 0},
    {"red-tanks", required_argument, NULL, 0},
    {"green-tank", required_argument, NULL, 0},
    {"red-tank", required_argument, NULL, 0},
    {"daemonize", no_argument, NULL, 'd'},
    {"pipe", required_argument, NULL, 'p'},
    {"round-time", required_argument, NULL, 0},
    {0, 0, 0, 0}
};

/* Help screen */
void usage()
{
    cout << "Usage:" << endl;

    cout << "\t" << "--area-size <N> <M>" << endl;
    cout << "\t\t" << "game area will have size <N> x <M>" << endl;

    cout << "\t" << "--green-tanks <N>" << endl;
    cout << "\t\t" << "creates <N> green tanks" << endl;

    cout << "\t" << "--red-tanks <N>" << endl;
    cout << "\t\t" << "creates <N> red tanks" << endl;

    cout << "\t" << "--green-tank <path>" << endl;
    cout << "\t\t" << "path to green tank program" << endl;

    cout << "\t" << "--red-tank <path>" << endl;
    cout << "\t\t" << "path to red tank program" << endl;

    cout << "\t" << "-d, --daemonize" << endl;
    cout << "\t\t" << "run world as daemon" << endl;

    cout << "\t" << "-p, --pipe <path>" << endl;
    cout << "\t\t" << "use <path> as a FIFO pipe for worldclient program" << endl;

    cout << "\t" << "-h, --help" << endl;
    cout << "\t\t" << "shows this help" << endl << endl;
}

/* Signal handlers */
volatile bool done = false;
volatile bool restart = false;

static void termhdl(int signo)
{
    if (signo != SIGQUIT && signo != SIGTERM && signo != SIGINT)
        syslog(LOG_ERR, "Error: invalid signal");
    else
        done = true;
}

static void resthdl(int signo)
{
    if (signo != SIGUSR1)
        syslog(LOG_ERR, "Error: invalid signal");
    else
        restart = true;
}

/* Option parsing */
struct worldOptions {
    int	areaX;
    int areaY;
    int greenCount;
    int redCount;
    string greenPath;
    string redPath;
    bool daemonize;
    string pipePath;
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
    bool gpth = false;
    bool rpth = false;
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
            case 4: // --green-tank
                options.greenPath = optarg;
                gpth = true;
                break;
            case 5: // --red-tank
                options.redPath = optarg;
                rpth = true;
                break;
            case 8: // --round-time
                options.roundTime = atoi(optarg);
                rndt = true;
            default:
                break;
            }
            break;

        case 'h':   // --help
            usage();
            exit(0);
        case 'd':   // --daemonize
            options.daemonize = true;
            break;
        case 'p':   // --pipe
            options.pipePath = optarg;
            ppth = true;
            break;
        default:
            usage();
            exit(1);
        }
    }

    if (!area || !gcnt || !rcnt || !gpth || !rpth || !rndt || !ppth) {
        cerr << "some required options were not provided" << endl;
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
        cerr << "invalid options provided" << endl;
        exit(1);
    }

    /* Check if there is another instance of world running */
    char *worldPidPath = "/var/run/world.pid";
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

    /* Create Pipe */
    FILE *namedPipe;

    mkfifo(options.pipePath, S_IRUSR | S_IWUSR);
    if ((namedPipe = open(options.pipePath, O_WRONLY)) == -1) {
        syslog(LOG_ERR, "open() fifo pipe failed: %s", strerror(errno));
        unlink(worldPidPath); //<< should go to try-catch block
        exit(1);
    }

    /* Set signal actions */
    struct sigaction termsa;
    sigemptyset(&termsa.sa_mask);
    termsa.sa_flags = 0;
    termsa.sa_handler = termhdl;
    sigaction(SIGQUIT, &termsa, NULL);
    sigaction(SIGINT, &termsa, NULL);
    sigaction(SIGTERM, &termsa, NULL);

    struct sigaction restsa;
    sigemptyset(&restsa.sa_mask);
    restsa.sa_flags = 0;
    restsa.sa_handler = resthdl;
    sigaction(SIGUSR1, &restsa, NULL);

    /* Run game */
    World world(options.areaX, options.areaY, options.redCount,
                options.greenCount, namedPipe, options.roundTime,
                options.greenPath, options.redTankPath);

    world.init();

    while (!done) {
        if (restart) {
            world.restart();
            restart = false;
        } else {
            world.performRound();
        }
    }

    /* Delete created files */
    unlink(pipePath);
    unlink(worldPidPath);

    return 0;
}
