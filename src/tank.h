#ifndef INTERNET_OF_TANKS_TANK_H
#define INTERNET_OF_TANKS_TANK_H

#include <utility>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>

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

    Tank(const Team &team, const char *const tankBinaryPath);

    virtual ~Tank()
    {
        close(readPipe);
        if (kill(pid, SIGINT) != 0) {
            syslog(LOG_WARNING, "Trying to kill tank process failed - kill(%d, %d): %s", pid, SIGINT, strerror(errno));
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
    pid_t pid;
    bool destroyed;
    int readPipe;
    Action action;
};


#endif //INTERNET_OF_TANKS_TANK_H
