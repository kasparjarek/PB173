#include "tank.h"

#include <netdb.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdexcept>
#include <thread>

std::condition_variable Tank::actionCV;
std::mutex Tank::actionMtx;

Tank::Tank(const Team &team)
    : team(team), action(UNDEFINED), actionBuffer{'n','o','n','o'}, sd_client(0), destroyed(false)
{
    currentAction = actionBuffer;
    if (sem_init(&readySem, 0, 0) == -1) {
        syslog(LOG_ERR, "sem_init() failed: %s", strerror(errno));
        throw std::runtime_error("Creating semaphore failed");
    }

    try {
        thread = new std::thread(&Tank::threadFnc, this);
    } catch (std::system_error error) {
        syslog(LOG_ERR, "Creating tank thread failed: %s", error.what());
        sem_destroy(&readySem);
        throw error;
    }
}

bool Tank::isDestroyed() const
{
    return destroyed;
}

Tank* Tank::markAsDestroyed()
{
    destroyed = true;
    return this;
}

const Action &Tank::getAction() const
{
    return action;
}

const Team &Tank::getTeam() const
{
    return team;
}

void Tank::notifyAllTanks()
{
    std::unique_lock<std::mutex> uniqueLock(actionMtx);
    Tank::actionCV.notify_all();
}

void Tank::threadFnc()
{
    std::unique_lock<std::mutex> uniqueLock(actionMtx, std::defer_lock);

    while (!destroyed) {

        uniqueLock.lock();
        if (sem_post(&readySem) != 0) {
            syslog(LOG_WARNING, "sem_post() failed: %s", strerror(errno));
        }
        actionCV.wait(uniqueLock);
        uniqueLock.unlock();

        if (destroyed) {
            break;
        }

        doAction();
    }
}

int Tank::waitForTank()
{
    if (sem_wait(&readySem) == -1) {
        syslog(LOG_WARNING, "sem_wait() failed: %s", strerror(errno));
        action = UNDEFINED;
        return -1;
    }
    return 0;
}

void Tank::doAction()
{
    this->action = parseAction(currentAction);
    if (sd_client != 0 && send(sd_client, currentAction, 2, 0) == -1) {
        syslog(LOG_ERR, "send() failed: %s", strerror(errno));
    }
    currentAction[0] = 'n';
    currentAction[1] = 'o';
    if (currentAction == actionBuffer)
        currentAction += 2;
    else
        currentAction -= 2;
}

Action Tank::parseAction(const char *actionStr)
{
    switch (actionStr[0]) {
    case 'f':
        switch (actionStr[1]) {
        case 'u':
            return FIRE_UP;
        case 'd':
            return FIRE_DOWN;
        case 'l':
            return FIRE_LEFT;
        case 'r':
            return FIRE_RIGHT;
        default:
            return UNDEFINED;
        }
    case 'm':
        switch (actionStr[1]) {
        case 'u':
            return MOVE_UP;
        case 'd':
            return MOVE_DOWN;
        case 'l':
            return MOVE_LEFT;
        case 'r':
            return MOVE_RIGHT;
        default:
            return UNDEFINED;
        }
    case 'n':
        if (actionStr[1] == 'o')
            return NO_ACTION;
    default:
        return UNDEFINED;
    }
}

void Tank::setNextAction(const char *actionStr)
{
    if (currentAction == actionBuffer) {
        actionBuffer[2] = actionStr[0];
        actionBuffer[3] = actionStr[1];
    } else {
        actionBuffer[0] = actionStr[0];
        actionBuffer[1] = actionStr[1];
    }
}

void Tank::_setActionToUndefined()
{
    this->action = UNDEFINED;
}

void Tank::setSocket(const sockaddr *addr, socklen_t addrlen)
{
    if ((sd_client = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
        throw std::runtime_error("socket() failed while assigning client to tank");
    }
    if (connect(sd_client, addr, addrlen) == -1) {
        syslog(LOG_ERR, "connect() failed: %s", strerror(errno));
        throw std::runtime_error("connect() failed while assigning client to tank");
    }
}
