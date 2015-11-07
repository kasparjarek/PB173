#include <sys/syslog.h>
#include <string.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <stdexcept>
#include <signal.h>
#include <unistd.h>
#include "tank.h"


Tank::Tank(const Team &team, const char *const tankBinaryPath)
    : team(team), tankBinaryPath(tankBinaryPath), destroyed(false), action(UNDEFINED)
{
    int pfd[2];

    if (pipe(pfd) == -1) {
        syslog(LOG_ERR, "pipe() failed: %s", strerror(errno));
        throw std::runtime_error("Creating pipe failed");
    }

    int err;
    if ((err = pthread_create(&thread, nullptr, tankThread, (void*) &pfd[1])) != 0) {
        syslog(LOG_ERR, "creating tank thread failed: %s", strerror(err));
        throw std::runtime_error("Creating tank thread failed");
    }

    readPipe = pfd[0];
}

bool Tank::isDestroyed() const
{
    return destroyed;
}

void Tank::markAsDestroyed()
{
    destroyed = true;
}

int Tank::fetchAction()
{
    if (pthread_kill(thread, SIGUSR2) != 0) {
        syslog(LOG_WARNING, "Sending signal to tank thread failed - kill(%d, %d): %s", pid, SIGUSR2, strerror(errno));
        action = UNDEFINED;
        return -1;
    }

    // parse action from pipe
    char buff[2];
    if (read(readPipe, buff, 2) == -1) {
        syslog(LOG_WARNING, "Reading from pipe failed - read(%d, buff, 2): %s", readPipe, strerror(errno));
        action = UNDEFINED;
        return -1;
    }

    switch (buff[0]) {
        // Move
        case 'm':
            switch (buff[1]) {
                case 'u':
                    action = MOVE_UP;
                    break;
                case 'd':
                    action = MOVE_DOWN;
                    break;
                case 'r':
                    action = MOVE_RIGHT;
                    break;
                case 'l':
                    action = MOVE_LEFT;
                    break;
                default:
                    action = UNDEFINED;
            }
            break;

        // Fire
        case 'f':
            switch (buff[1]) {
                case 'u':
                    action = FIRE_UP;
                    break;
                case 'd':
                    action = FIRE_DOWN;
                    break;
                case 'r':
                    action = FIRE_RIGHT;
                    break;
                case 'l':
                    action = FIRE_LEFT;
                    break;
                default:
                    action = UNDEFINED;
            }
            break;

        default:
            action = UNDEFINED;
    }

    if (action == UNDEFINED) {
        syslog(LOG_WARNING, "Read undefined action from pipe: '%c%c'", buff[0], buff[1]);
    }
    return 0;
}

const Action &Tank::getAction() const
{
    return action;
}

const Team &Tank::getTeam() const
{
    return team;
}

pid_t Tank::getThread() const
{
    return (pid_t) thread;
}
