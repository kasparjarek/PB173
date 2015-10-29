#include <iostream>
#include <getopt.h>
#include "world.h"

using namespace std;

extern char *optarg;
extern int optind;

const char *ARGS = "hdp";
const struct option LONG_ARGS[] = {
    {"green-tanks", required_argument, NULL, 'g'},
    {"red-tanks", required_argument, NULL, 'r'},
    {"total-respawn", required_argument, NULL, 't'},
    {"area-size", required_argument, NULL, 'a'},
    {"daemonize", no_argument, NULL, 'd'},
    {"pipe", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};



void usage()
{
    cout << "Usage:" << endl;
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

static World *world = nullptr;

static void terminate(int signo)
{
    if (world != nullptr)
        world->stop();
}

int main(int argc, char *argv[])
{
    char opt = 0;
    int areaX = -1;
    int areaY = -1;
    int greenCount = -1;
    int redCount = -1;
    int totalRespawn = -1;

    while ((opt = getopt_long(argc, argv, ARGS, LONG_ARGS, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        case 'g':
            greenCount = atoi(optarg);
            break;
        case 'r':
            redCount = atoi(optarg);
            break;
        case 't':
            totalRespawn = atoi(optarg);
            break;
        case 'a':
            areaX = atoi(optarg);
            areaY = atoi(argv[optind++]);
            break;
        default:
            usage();
            exit(1);
        }
    }

    //world = new World(areaX, areaY, totalRespawn, redCount, greenCount);

    struct sigaction termsa;
    sigemptyset(&termsa.sa_mask);
    termsa.sa_flags = 0;
    termsa.sa_handler = terminate;

    sigaction(SIGQUIT, &termsa, NULL);
    sigaction(SIGINT, &termsa, NULL);
    sigaction(SIGTERM, &termsa, NULL);

    //world.start();

    //delete world;

    return 0;
}
