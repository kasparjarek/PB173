#ifndef INTERNET_OF_TANKS_TANK_H
#define INTERNET_OF_TANKS_TANK_H

#include <atomic>
#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>
#include <utility>
#include <semaphore.h>
#include <thread>
#include <condition_variable>

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

    Tank(const Team &team);

    virtual ~Tank()
    {
        threadDone = true;
        thread->detach();
        delete thread;

        sem_destroy(&actionSem);
    }

    /**
     * Check if the tank is marked as destroyed
     */
    bool isDestroyed() const;

    /**
     * Mark this tank as destroyed
     */
    void markAsDestroyed();

    /**
     * Parse action after calling requireActionsFromAllTanks method.
     * This action can be retrieved by getAction method.
     * If communication with tank failed or data retrieved from tank doesn't match any familiar action
     * return -1 and set action to UNDEFINED otherwise return 0 and set action to proper value.
     */
    int fetchAction();

    /**
     * Get action parsed by fetchAction method
     */
    const Action & getAction() const;

    /**
     * Get team membership
     */
    const Team & getTeam() const;

    /**
     * Contact all tanks and ask then about the action.
     */
    static void requireActionsFromAllTanks();

private:

    static std::condition_variable actionCV;
    static std::mutex actionMtx;

    Team team;
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
