/* Rename file to tank-thread maybe? */

#include <ctime>
#include <csignal>
#include <cstdlib>
#include <getopt.h>
#include <pthread.h>
#include <syslog.h>
#include <unistd.h>
#include "tank.h"

using namespace std;

void sigHandler(int signo)
{
    if (signo != SIGUSR2) {
        syslog(LOG_ERR, "Error: invalid signal");
    } else {
        const bool t = true;
        pthread_setspecific(tankAction, &t);
    }
}

/**
 * @brief doAction generates an action and writes it to the pipe
 * @param writePipe
 */
void doAction(int writePipe)
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

    write(writePipe, buf, 2);
}


void *tankThread(void *arg)
{
    /* Block signals intended for world */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);

    srand(pthread_self());
    int writePipe = *(int*)arg;

    /* Set signal handlers */
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigHandler;

    sigaction(SIGUSR2, &act, NULL);

    /* Wait for signal from world and then do some action */
    while (true) {
        pause();
        if (*(bool*)pthread_getspecific(tankAction)) {
            doAction(writePipe);
            const bool f = false;
            pthread_setspecific(tankAction, &f);
        }
    }

    return 0;
}
