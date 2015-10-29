#include <sys/syslog.h>
#include <string.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <stdexcept>
#include <signal.h>
#include <unistd.h>
#include "tank.h"


Tank::Tank(const Team &team, const char *const tankBinaryPath, int areaX, int areaY)
    : team(team), tankBinaryPath(tankBinaryPath), destroyed(false), action(UNDEFINED)
{
    int pfd[2];

    if (pipe(pfd) == -1) {
        syslog(LOG_ERR, "pipe() failed: %s", strerror(errno));
        throw std::runtime_error("Creating pipe failed");
    }

    pid_t tankPid = fork();
    if (tankPid == -1) {
        syslog(LOG_ERR, "Creating new tank failed: %s", strerror(errno));
        throw std::runtime_error("Fork failed");
    }
    // Child
    else if (tankPid == 0) {
        close(pfd[0]);
        close(STDOUT_FILENO);
        dup2(pfd[1], STDOUT_FILENO);

        std::string x = std::to_string(areaX);
        std::string y = std::to_string(areaY);

        if (execl(tankBinaryPath, tankBinaryPath, "--area-size", x.c_str(), y.c_str(),  NULL) == -1) {
            syslog(LOG_ERR, "execl() failed: %s", strerror(errno));
        }
        exit(-1);
    }

    // Parent
    close(pfd[1]);
    readPipe = pfd[0];
    pid = tankPid;
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
    if (kill(pid, SIGUSR2) != 0) {
        syslog(LOG_WARNING, "Sending signal to tank process failed - kill(%d, %d): %s", pid, SIGUSR2, strerror(errno));
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
pid_t Tank::getPid() const
{
    return pid;
}
