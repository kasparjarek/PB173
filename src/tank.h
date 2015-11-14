#ifndef INTERNET_OF_TANKS_TANK_H
#define INTERNET_OF_TANKS_TANK_H

#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>
#include <utility>
#include <sys/semaphore.h>
#include <thread>

enum Team
{
    GREEN, RED
};

enum Action
{
    UNDEFINED,
    MOVE_UP, MOVE_DOWN, MOVE_RIGHT, MOVE_LEFT,
    FIRE_UP, FIRE_DOWN, FIRE_RIGHT, FIRE_LEFT
};


class Tank
{
public:

    Tank(const Team &team, const char *const tankBinaryPath /* UNUSED */);

    virtual ~Tank()
    {
        threadDone = true;
        thread->detach();
        delete thread;

        sem_destroy(&actionSem);
    }

    /**
     *
     */
    bool isDestroyed() const;

    /**
     * Mark this tank as destroyed
     */
    void markAsDestroyed();

    /**
     * Contact tank and ask him about the action. This action can be retrieved by getAction method.
     * If communication with tank failed, return -1 and set action to UNDEFINED,
     * otherwise return 0 and set action to proper value.
     * Action can be also UNDEFINED when data retrieved from tank doesn't match any familiar action.
     */
    int fetchAction();

    const Action & getAction() const;

    const Team & getTeam() const;

    static void requireActionsFromAllTanks();


private:

    static std::condition_variable actionCV;
    static std::mutex actionMtx;

    Team team;
    const char *const tankBinaryPath;
    bool destroyed;
    char actionMsg[2];
    sem_t actionSem;
    Action action;

    std::thread *thread;
    std::atomic_bool threadDone;

    void threadFnc();
    void doAction();
};


#endif //INTERNET_OF_TANKS_TANK_H
