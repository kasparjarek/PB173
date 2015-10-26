#ifndef INTERNET_OF_TANKS_WORLD_H
#define INTERNET_OF_TANKS_WORLD_H

#include <set>
#include <unistd.h>
#include "tankbean.h"
#include "gameboard.h"

struct compareTankBeanByPosition
{
    bool operator()(const TankBean *a, const TankBean *b) const
    {
        return a->position < b->position;
    }
};

struct compareTankBeanByPid
{
    bool operator()(const TankBean *a, const TankBean *b) const
    {
        return a->pid < b->pid;
    }
};

class World
{
public:
    World(int areaX,
          int areaY,
          int totalRespawn,
          int redCount,
          int greenCount);

    virtual ~World()
    {
        freeTanks();
    }

    /**
     * Start game. Game board is displayed and tank processes are created.
     * Wait for terminating tank processes and respawning new, until totalRespawn
     * is reached.
     * @return -1 when occurs some error, 0 otherwise
     */
    int start();

private:
    int areaX;
    int areaY;
    int totalRespawn;
    int currentRespawn;
    int redCount;
    int greenCount;
    Gameboard gameboard;

    std::set<TankBean *, compareTankBeanByPosition> tanksByPosition;

    std::set<TankBean *, compareTankBeanByPid> tanksByPid;

    /**
     * Create tank - generate random position for tank, create child process
     * and add new TankBean into sets maintaining tanks.
     * @return pointer to created Tank or nullptr if creating failed
     */
    TankBean *createTank(Team team);

    /**
     * Create several tanks using createTank method. If invoking one of this method fails
     * return -1 and clear all tanks.
     * @return -1 if fails creating one of tanks, 0 otherwise
     */
    int createTanks(Team team, int count);

    /**
     * Remove tank with given pid from memory and from the sets maintaining tanks.
     * Create new tank replacing the old one (belonging to the same team).
     * @return Nullptr if creating new tank failed or tank with given pid wasn't found,
     * pointer to created Tank otherwise.
     */
    TankBean *respawnTank(pid_t pid);

    /**
     * Empty sets maintaining tanks and free memory occupied by this tanks.
     */
    void freeTanks();
};

#endif //INTERNET_OF_TANKS_WORLD_H

