#ifndef INTERNET_OF_TANKS_TANK_H
#define INTERNET_OF_TANKS_TANK_H

#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <thread>
#include <utility>
#include <vector>

enum Team
{
    GREEN, RED
};

enum Action
{
    UNDEFINED,
    MOVE_UP, MOVE_DOWN, MOVE_RIGHT, MOVE_LEFT,
    FIRE_UP, FIRE_DOWN, FIRE_RIGHT, FIRE_LEFT,
    NO_ACTION
};


class Tank
{
public:

    Tank(const Team &team);

    virtual ~Tank()
    {
        close(sd_client);
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

    void setNextAction(const char* actionStr);

    void setSocket(const struct sockaddr* addr, socklen_t addrlen);

    /**
     * Contact all tanks and ask then about the action.
     */
    static void notifyAllTanks();

    int waitForTank();

    void joinThread();

private:

    static std::condition_variable actionCV;
    static std::mutex actionMtx;

    Team team;
    sem_t readySem;
    Action action;
    char actionBuffer[4];
    char* currentAction;

    int sd_client;      //<< socket descriptor to tankclient

    std::thread *thread;
    std::atomic_bool destroyed;

    void threadFnc();
    void doAction();
    Action parseAction(const char* actionStr);
};


#endif //INTERNET_OF_TANKS_TANK_H
