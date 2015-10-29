#include <ctime>
#include <csignal>
#include <cstdlib>
#include <getopt.h>
#include <syslog.h>
#include <unistd.h>
#include "tank.h"

using namespace std;

const struct option LONG_ARGS[] = {
    {"area-size", required_argument, NULL, 'a'},
    {0, 0, 0, 0}
};

void doAction(int signo)
{
    if (signo != SIGUSR2)
        syslog(LOG_ERR, "Error: invalid signal");

    char buf[2];

    enum Action action = static_cast<enum Action>((rand() % 8)  + 1);
    switch (action) {
    case MOVE_UP:
        strncpy(buf, "mu", 2);
        break;
    case MOVE_DOWN:
        strncpy(buf, "md", 2);
        break;
    case MOVE_RIGHT:
        strncpy(buf, "mr", 2);
        break;
    case MOVE_LEFT:
        strncpy(buf, "ml", 2);
        break;
    case FIRE_UP:
        strncpy(buf, "fu", 2);
        break;
    case FIRE_DOWN:
        strncpy(buf, "fd", 2);
        break;
    case FIRE_RIGHT:
        strncpy(buf, "fr", 2);
        break;
    case FIRE_LEFT:
        strncpy(buf, "fl", 2);
    default:
        ;
    }

    write(STDOUT_FILENO, buf, 2);
}


int main(int argc, char *argv[])
{
    char opt = 0;
    int areaX, areaY;

    while ((opt = getopt_long(argc, argv, "", LONG_ARGS, NULL)) != -1) {
        switch (opt) {
        case 'a':
            areaX = atoi(optarg);
            areaY = atoi(argv[optind++]);
            break;
        default:
            exit(1);
        }
    }

    srand(getpid());

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = doAction;

    sigaction(SIGUSR2, &act, NULL);

    while (true)
        pause();

    return 0;
}
