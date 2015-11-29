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
bool Tank::notifyOk;

Tank::Tank(const Team &team)
    : team(team), destroyed(false), action(UNDEFINED), threadDone(false)
{
    if (sem_init(&actionSem, 0, 0) == -1) {
        syslog(LOG_ERR, "sem_init() failed: %s", strerror(errno));
        throw std::runtime_error("Creating semaphore failed");
    }
    if (sem_init(&readySem, 0, 0) == -1) {
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
    if (sem_wait(&actionSem) == -1) {
        syslog(LOG_WARNING, "sem_wait() failed: %s", strerror(errno));
        action = UNDEFINED;
        return -1;
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
    std::unique_lock<std::mutex> uniqueLock(actionMtx);
    //notifyOk = true;
    Tank::actionCV.notify_all();
    uniqueLock.unlock();
}

void Tank::threadFnc()
{
    std::unique_lock<std::mutex> uniqueLock(actionMtx, std::defer_lock);

    while (!threadDone) {

        uniqueLock.lock();
        sem_post(&readySem);
        actionCV.wait_for(uniqueLock, std::chrono::seconds(2));
        uniqueLock.unlock();

        if (!threadDone)
            doAction();
    }
}

void Tank::waitForTank()
{
    sem_wait(&readySem);
}

void Tank::doAction()
{
    // TODO fix calling rand in thread (maybe rand_r can be solution)
    this->action = static_cast<enum Action>((rand() % 8)  + 1);

    if (sem_post(&actionSem) != 0) {
        syslog(LOG_WARNING, "sem_post() failed: %s", strerror(errno));
    }
}
