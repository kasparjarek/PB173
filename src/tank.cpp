#include <sys/syslog.h>
#include <string.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <stdexcept>
#include <signal.h>
#include <unistd.h>
#include <thread>
#include "tank.h"

std::condition_variable Tank::actionCV;
std::mutex Tank::actionMtx;

Tank::Tank(const Team &team, const char *const tankBinaryPath)
    : team(team), tankBinaryPath(tankBinaryPath), destroyed(false), action(UNDEFINED), threadDone(false)
{
    if (sem_init(&actionSem, 0, 0) == -1) {
        syslog(LOG_ERR, "sem_init() failed: %s", strerror(errno));
        throw std::runtime_error("Creating semaphore failed");
    }

    try {
        thread = new std::thread(&Tank::threadFnc, this);
    } catch (std::system_error error) {
        syslog(LOG_ERR, "Creating tank thread failed: %s", error.what());
        sem_destroy(&actionSem);
        throw error;
    }
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
    // parse action from pipe
    if (sem_wait(&actionSem) == -1) {
        syslog(LOG_WARNING, "sem_wait() failed: %s", strerror(errno));
        action = UNDEFINED;
        return -1;
    }

    switch (actionMsg[0]) {
        // Move
        case 'm':
            switch (actionMsg[1]) {
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
            switch (actionMsg[1]) {
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
        syslog(LOG_WARNING, "Read undefined action from array: '%c%c'", actionMsg[0], actionMsg[1]);
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

void Tank::requireActionsFromAllTanks()
{
    Tank::actionCV.notify_all();
}

void Tank::threadFnc()
{
    std::unique_lock<std::mutex> uniqueLock(actionMtx);

    while(true) {
        if (actionCV.wait_for(uniqueLock, std::chrono::seconds(4)) == std::cv_status::no_timeout) {
            doAction();
        }

        if (threadDone) {
            return;
        }
    }
}

void Tank::doAction()
{
    // TODO fix calling rand in thread (maybe rand_r can be solution)
    enum Action action = static_cast<enum Action>((rand() % 8)  + 1);

    switch (action) {
        case MOVE_UP:
            strncpy(actionMsg, "mu", 2);
            break;
        case MOVE_DOWN:
            strncpy(actionMsg, "md", 2);
            break;
        case MOVE_RIGHT:
            strncpy(actionMsg, "mr", 2);
            break;
        case MOVE_LEFT:
            strncpy(actionMsg, "ml", 2);
            break;
        case FIRE_UP:
            strncpy(actionMsg, "fu", 2);
            break;
        case FIRE_DOWN:
            strncpy(actionMsg, "fd", 2);
            break;
        case FIRE_RIGHT:
            strncpy(actionMsg, "fr", 2);
            break;
        case FIRE_LEFT:
            strncpy(actionMsg, "fl", 2);
        default:
            ;
    }

    if (sem_post(&actionSem) != 0) {
        syslog(LOG_WARNING, "sem_post() failed: %s", strerror(errno));
    }
}
