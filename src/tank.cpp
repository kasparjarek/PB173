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

Tank::Tank(const Team &team)
    : team(team), action(UNDEFINED), destroyed(false)
{
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

        doAction();

        uniqueLock.lock();
        if (sem_post(&readySem) != 0) {
            syslog(LOG_WARNING, "sem_post() failed: %s", strerror(errno));
        }
        actionCV.wait(uniqueLock);
        uniqueLock.unlock();
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
    // TODO fix calling rand in thread (maybe rand_r can be solution)
    this->action = static_cast<enum Action>((rand() % 8)  + 1);
}
