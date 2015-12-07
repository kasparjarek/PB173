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
#include <vector>

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
        sem_destroy(&readySem);
        thread->join();
        delete thread;
    }

    /**
     * Check if the tank is marked as destroyed
     */
    bool isDestroyed() const;

    /**
     * Mark this tank as destroyed
     */
    Tank *markAsDestroyed();

    /**
     * Get action parsed by fetchAction method
     */
    const Action & getAction() const;

    /**
     * Get team membership
     */
    const Team & getTeam() const;

    void _setActionToUndefined();

    /**
     * Contact all tanks and ask then about the action.
     */
    static void notifyAllTanks();

    int waitForTank();

private:

    static std::condition_variable actionCV;
    static std::mutex actionMtx;

    Team team;
    sem_t readySem;
    Action action;

    std::thread *thread;
    std::atomic_bool destroyed;

    void threadFnc();
    void doAction();
};


#endif //INTERNET_OF_TANKS_TANK_H
