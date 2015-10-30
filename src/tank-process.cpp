#include <ctime>
#include <csignal>
#include <cstdlib>
#include <getopt.h>
#include <syslog.h>
#include <unistd.h>
#include "tank.h"

using namespace std;

volatile bool action = false;

void acthdl(int signo)
{
    if (signo != SIGUSR2)
        syslog(LOG_ERR, "Error: invalid signal");
    else
        action = true;
}

void doAction()
{

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
    srand(getpid());

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = acthdl;

    sigaction(SIGUSR2, &act, NULL);

    while (true) {
        pause();
        if (action) {
            doAction();
            action = false;
        }
    }

    return 0;
}
