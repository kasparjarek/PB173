#ifndef INTERNET_OF_TANKS_WORLD_H
#define INTERNET_OF_TANKS_WORLD_H

#include <set>
#include <unistd.h>
#include <string>
#include <map>
#include "tank.h"

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
     * Start game. Game is running until someone call stop method, then the game is reset and ready for new start.
     * @return -1 when occurs some error, 0 otherwise
     */
    int init();

    void performRound();

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

    std::map<int, std::map<int, Tank*> > tanks;   // row, column


    /**
     * Create tank - generate random position for tank, create child process
     * and add new Tank pointer into maps maintaining tanks.
     * @return pointer to created tank or nullptr if creating failed
     */
    Tank * createTank(Team team);

    /**
     * Create several tanks using createTank method. If invoking one of this method fails
     * return -1 and clear all tanks.
     * @return -1 if fails creating one of tanks, 0 otherwise
     */
    int createTanks(Team team, int count);

    /**
     * Iterate through all tanks ale perform theirs actions
     */
    int performActions();

    int printGameBoard();

    /**
     * Empty map maintaining tanks and free memory occupied by this tank.
     */
    void clearTanks();

    void logTankHit(pid_t aggressorPid, int aggressorX, int aggressorY,
                    pid_t victimPid, int victimX, int victimY);

    void logTankRolledOffTheMap(pid_t pid, int x, int y);

    void logTankCrash(pid_t aggressorPid, int aggressorX, int aggressorY,
                      pid_t victimPid, int victimX, int victimY);
};

#endif //INTERNET_OF_TANKS_WORLD_H

