#ifndef INTERNET_OF_TANKS_TANK_H
#define INTERNET_OF_TANKS_TANK_H

#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>
#include <utility>

pthread_key_t tankAction;

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
        close(readPipe);
        int err;
        if ((err = pthread_kill(thread, SIGTERM) != 0)) {
            syslog(LOG_WARNING, "Trying to kill tank thread failed - kill(%d, %d): %s", (pid_t) thread, SIGTERM, strerror(err));
        }
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

    pid_t getPid() const;

private:
    Team team;
    const char *const tankBinaryPath;
    pthread_t thread;
    bool destroyed;
    int readPipe;
    Action action;
};


#endif //INTERNET_OF_TANKS_TANK_H
