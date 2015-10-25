#include <iostream>
#include <getopt.h>
#include "world.h"

using namespace std;

extern char *optarg;
extern int optind;

const char *ARGS = "h";
const struct option LONG_ARGS[] = {
    {"green-tanks", required_argument, NULL, 'g'},
    {"red-tanks", required_argument, NULL, 'r'},
    {"total-respawn", required_argument, NULL, 't'},
    {"area-size", required_argument, NULL, 'a'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};



void usage()
{
    cout << "Usage:\n";
    cout << "\t--green-tanks <N>\n";
    cout << "\t\tcreates <N> green tanks\n\n";
    cout << "\t--red-tanks <N>\n";
    cout << "\t\tcreates <N> red tanks\n\n";
    cout << "\t--total-respawn <N>\n";
    cout << "\t\ta total of <N> tanks will be respawned\n\n";
    cout << "\t--area-size <N> <M>\n";
    cout << "\t\tgame area will have size <N> x <M>\n\n";
    cout << "\t-h, --help\n";
    cout << "\t\tshows this help\n\n";
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
            abort();
        }
    }

    World world(areaY, areaX, totalRespawn, redCount, greenCount);
    world.start();

    return 0;
}
