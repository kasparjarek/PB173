#include <iostream>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "world.h"

using namespace std;

extern char *optarg;
extern int optind;

const char *ARGS = "hdp";
const struct option LONG_ARGS[] = {
    {"help", no_argument, NULL, 'h'},
    {"area-size", required_argument, NULL, 'a'},
    {"green-tanks", required_argument, NULL, 'g'},
    {"red-tanks", required_argument, NULL, 'r'},
    {"green-tank", required_argument, NULL, 'i'},
    {"red-tank", required_argument, NULL, 'j'},
    {"daemonize", no_argument, NULL, 'd'},
    {"pipe", required_argument, NULL, 'p'},
    {"round-time", required_argument, NULL, 't'},
    {0, 0, 0, 0}
};

const char *WORLD_PID_PATH = "/var/run/world.pid";

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

int main(int argc, char *argv[])
{
    /* Check if there is another instance of world running */
    int wpfd;
    if ((wpfd = open(WORLD_PID_PATH, O_RDWR)) == -1) {
        syslog(LOG_ERR, "cannot open %s: %s", WORLD_PID_PATH, strerror(errno));
        exit(1);
    }

    pid_t savedPid;
    if (read(wpfd, &savedPid, sizeof(pid_t)) == 0) {
        pid_t myPid = getpid();
        write(wpfd, &myPid, sizeof(pid_t));
        close(wpfd);
    } else {
        if (savedPid != getpid()) {
            syslog(LOG_INFO, "world pid %d is already running", savedPid);
            exit(0);
        }
    }

    char opt = 0;

    int areaX = -1;
    int areaY = -1;
    int redCount = -1;
    int greenCount = -1;
    char *fifoPath;
    int namedPipe;
    useconds_t roundTime = 0;
    char *redTankPath;
    char *greenTankPath;

    /* Process arguments */
    while ((opt = getopt_long(argc, argv, ARGS, LONG_ARGS, NULL)) != -1) {
        switch (opt) {
        case 'h':   // --help
            usage();
            return 0;
        case 'a':   // --area-size
            areaX = atoi(optarg);
            areaY = atoi(argv[optind++]);
            break;
        case 'g':   // --green-tanks
            greenCount = atoi(optarg);
            break;
        case 'r':   // --red-tanks
            redCount = atoi(optarg);
            break;
        case 'i':   // --green-tank
            greenTankPath = optarg;
            break;
        case 'j':   // --red-tank
            redTankPath = optarg;
            break;
        case 'd':   // --daemonize
            if (daemon(1, 0) != 0) {
                syslog(LOG_ERR, "daemon() failed: %s", strerror(errno));
                exit(1);
            }
            openlog(NULL, 0, LOG_DAEMON);
            break;
        case 'p':   // --pipe
            fifoPath = optarg;
            mkfifo(fifoPath, S_IRUSR | S_IWUSR);
            if ((namedPipe = open(fifoPath, O_WRONLY)) == -1) {
                syslog(LOG_ERR, "open() fifo pipe failed: %s", strerror(errno));
                exit(1);
            }
            break;
        case 't':   // --round-time
            roundTime = atoi(optarg);
            break;
        default:
            usage();
            exit(1);
        }
    }

    // TODO: do we have all the necessary arguments?

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
    World world(areaX, areaY, redCount, greenCount, namedPipe, roundTime, greenTankPath, redTankPath);

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
    unlink(fifoPath);
    unlink(WORLD_PID_PATH);

    return 0;
}
