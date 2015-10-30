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


// TODO: rewrite usage
void usage()
{
    cout << "Usage:" << endl;
    cout << "REWIRTE" << endl;
    cout << "\t" << "--green-tanks <N>" << endl;
    cout << "\t\t" << "creates <N> green tanks" << endl << endl;
    cout << "\t" << "--red-tanks <N>" << endl;
    cout << "\t\t" << "creates <N> red tanks" << endl << endl;
    cout << "\t--total-respawn <N>" << endl;
    cout << "\t\t" << "a total of <N> tanks will be respawned" << endl << endl;
    cout << "\t" << "--area-size <N> <M>" << endl;
    cout << "\t\t" << "game area will have size <N> x <M>" << endl << endl;
    cout << "\t" << "-h, --help" << endl;
    cout << "\t\t" << "shows this help" << endl << endl;
}

volatile bool done = false;
volatile bool restart = false;

static void termhdl(int signo)
{
    done = true;
}

static void resthdl(int signo)
{
    restart = true;
}

int main(int argc, char *argv[])
{
    // TODO: world.pid
    char opt = 0;

    int areaX = -1;
    int areaY = -1;
    int redCount = -1;
    int greenCount = -1;
    int totalRespawn = -1;
    char *fifoPath;
    int namedPipe;
    useconds_t roundTime = 0;
    char *redTankPath;
    char *greenTankPath;

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
            if (daemon(1, 0) != 0)
                syslog(LOG_ERR, "daemon() failed: %s", strerror(errno));
            openlog(NULL, 0, LOG_DAEMON);
            break;
        case 'p':   // --pipe
            fifoPath = optarg;
            mkfifo(fifoPath, S_IRUSR | S_IWUSR);
            if ((namedPipe = open(fifoPath, O_WRONLY)) == -1)
                syslog(LOG_ERR, "open() fifo pipe failed: %s", strerror(errno));
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

    World world(areaX, areaY, redCount, greenCount, namedPipe, roundTime, greenTankPath, redTankPath);

    world.start();

    while (!done) {
        if (restart) {
            world.restart();
            restart = 0;
        } else {
            world.run();
        }
    }

    world.stop();

    return 0;
}
