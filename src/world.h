#ifndef INTERNET_OF_TANKS_WORLD_H
#define INTERNET_OF_TANKS_WORLD_H

#include <set>
#include <unistd.h>
#include <string>
#include <map>
#include "tank.h"

using namespace std;


class World
{
public:
    World(int areaX,
          int areaY,
          int redCount,
          int greenCount,
          int namedPipe,
          useconds_t roundTime,
          const char *const greenTankPath,
          const char *const redTankPath);

    virtual ~World()
    {
        clearTanks();
    }

    /**
     * Start game. Game board is displayed and tank processes are created.
     * Wait for terminating tank processes and respawning new, until totalRespawn
     * is reached.
     * @return -1 when occurs some error, 0 otherwise
     */
    int start();

    int restart();

private:
    int areaX;
    int areaY;
    int redCount;
    int greenCount;
    int namedPipe;
    useconds_t roundTime;
    const char *const greenTankPath;
    const char *const redTankPath;
    unsigned int roundCount;

    map<int, map<int, Tank*> > tanks;


    /**
     * Create tank - generate random position for tank, create child process
     * and add new Tank pointer into sets maintaining tanks.
     * @return pointer to created tank or nullptr if creating failed
     */
    Tank * createTank(Team team);

    /**
     * Create several tanks using createTank method. If invoking one of this method fails
     * return -1 and clear all tanks.
     * @return -1 if fails creating one of tanks, 0 otherwise
     */
    int createTanks(Team team, int count);

    int performGameRound();

    void logTankHit(pid_t aggressorPid, int aggressorX, int aggressorY,
                    pid_t victimPid, int victimX, int victimY);

    void logTankRolledOffTheMap(pid_t pid, int x, int y);

    void logTankCrash(pid_t aggressorPid, int aggressorX, int aggressorY,
                      pid_t victimPid, int victimX, int victimY);

    /**
     * Empty map maintaining tanks and free memory occupied by this tanks.
     */
    void clearTanks();
};

#endif //INTERNET_OF_TANKS_WORLD_H

