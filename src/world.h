#ifndef INTERNET_OF_TANKS_WORLD_H
#define INTERNET_OF_TANKS_WORLD_H

#include "tank.h"

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

class SockAddrComparator
{
public:
    bool operator() (const struct sockaddr_in & a, const struct sockaddr_in & b) const
    {
        if (a.sin_addr.s_addr == b.sin_addr.s_addr) {
            return a.sin_port < b.sin_port;
        }
        return a.sin_addr.s_addr < b.sin_addr.s_addr;
    }
};

class World
{
public:
    World(int areaX,
          int areaY,
          int redCount,
          int greenCount,
          std::string & namedPipe,
          useconds_t roundTime);

    virtual ~World()
    {
        close(sd_listen);
        clearTanks();
    }

    /**
     * Initialization the game.
     * Create new tanks according to redCount and greenCount parameters which has been set in constructor.
     * Set roundCount to zero and delete all previous tanks.
     * @throw runtime_error when some error occurs
     */
    void init();

    /**
     * Perform one round. Increase roundCount by one.
     * Get actions from tank threads, parse them and perform them. After that wait for roundTime micro seconds.
     */
    void performRound();

private:
    int areaX;
    int areaY;
    int redCount;
    int greenCount;
    std::ofstream namedPipe;    //<< pipe to worldclient
    useconds_t roundTime;
    unsigned int roundCount;

    int sd_listen;             //<< listening socket descriptor

    std::map<int, std::map<int, Tank*> > tanks;   // row, column

    std::map<struct sockaddr_in, Tank*, SockAddrComparator> addrToTank;

    std::vector<Tank*> freeTanks;


    /**
     * Create tank - generate random position for tank, create new thread
     * and add new Tank pointer into maps maintaining tanks.
     * @return pointer to created tank
     * @throw runtime_error if creating tank fail
     */
    Tank * createTank(Team team);

    /**
     * Create several tanks using createTank method.
     * @throw runtime_error if creating tank fail
     */
    void createTanks(Team team, int count);

    /**
     * Prepare for listening to connection from tankclients
     * @throw runtime_error if setting the socket fails
     */
    void setListenSocket();

    void receiveMessages();

    /**
     * Iterate through all tanks ale perform theirs actions
     */
    int performActions();

    /**
     * Print game state into namedPipe
     */
    int printGameBoard();

    /**
     * Empty map maintaining tanks and free memory occupied by this tank.
     * Destructor of this tanks should terminate their threads.
     */
    void clearTanks();

    /**
     * Wait for ready semaphore on all tanks. After this, all tanks are ready to receive notify_all.
     */
    void waitForAllTanks();

    /**
     * Log 'Tank hit' event into syslog
     */
    void logTankHit(int aggressorX, int aggressorY, int victimX, int victimY);

    /**
     * Log 'Tank rolled off the map' event into syslog
     */
    void logTankRolledOffTheMap(int x, int y);

    /**
     * * Log 'Tank crash' event into syslog
     */
    void logTankCrash(int aggressorX, int aggressorY, int victimX, int victimY);
};

#endif //INTERNET_OF_TANKS_WORLD_H

