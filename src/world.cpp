#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <sys/wait.h>
#include <syslog.h>
#include "world.h"

using namespace std;

World::World(int areaX,
             int areaY,
             int totalRespawn,
             int redCount,
             int greenCount)
    : areaX(areaX), areaY(areaY), totalRespawn(totalRespawn), currentRespawn(0),
      redCount(redCount), greenCount(greenCount)
{
    srand((unsigned int) time(NULL));

    if (areaX < 0 || areaY < 0 || redCount < 0 || greenCount < 0
            || totalRespawn < 0 || (areaY * areaX < redCount + greenCount)) {
        throw runtime_error("invalid parameters");
    }
}

int World::start()
{

    // TODO: marty - create game board

    if (createTanks(Team::GREEN, greenCount) != 0) {
        return -1;
    }

    if (createTanks(Team::RED, redCount) != 0) {
        return -1;
    }


    int status = 0;
    TankBean tank;
    pid_t tankPid = 0;
    while (totalRespawn > currentRespawn) {
        tankPid = wait(&status);
        if (respawnTank(tankPid) == nullptr) {
            freeTanks();
            return -1;
        }
        currentRespawn++;
    }

    freeTanks();
    return 0;
}

void World::freeTanks()
{
    for (TankBean *i : tanksByPid) {
        delete i;
    }

    tanksByPid.clear();
    tanksByPosition.clear();
}

TankBean * World::createTank(Team team)
{
    TankBean *newTank = new TankBean();
    newTank->team = team;

    do {
        newTank->position = Coordinates(rand() % areaX, rand() % areaY);
    }
    while (!tanksByPosition.insert(newTank).second);


    pid_t tankPid = fork();

    if (tankPid == -1) {
        syslog(LOG_ERR, "Creating new tank failed: %s", strerror(errno));
        tanksByPosition.erase(newTank);
        delete newTank;
        return nullptr;
    }
    // Child
    else if (tankPid == 0) {
        /* TODO: change sleep-min and sleep-max */
        if (execl("tank", "tank", "--sleep-min", "1", "--sleep-max", "3", NULL) == -1) {
            syslog(LOG_ERR, "execl() failed: %s", strerror(errno));
        }
        exit(-1);
    }

    // Parent
    newTank->pid = tankPid;
    if (!tanksByPid.insert(newTank).second) {
        syslog(LOG_WARNING, "Tank with pid %d already exist in tanksByPid set. This tank is replaced.", tankPid);

        // Remove outdated tank from tanksByPid set
        auto outdatedTank = tanksByPid.insert(newTank).first;
        delete *outdatedTank;
        tanksByPid.erase(outdatedTank);

        tanksByPid.insert(newTank);
    }

    // TODO: marty - add tank
    return newTank;
}

int World::createTanks(Team team, int count)
{
    for (int i = 0; i < count; ++i) {
        if (createTank(team) == nullptr) {
            freeTanks();
            return -1;
        }
    }
    return 0;
}

TankBean * World::respawnTank(pid_t tankPid)
{
    TankBean requiredTank;
    requiredTank.pid = tankPid;
    auto iter = tanksByPid.find(&requiredTank);
    if (iter == tanksByPid.end()) {
        syslog(LOG_NOTICE, "tanksByPid set doesn't containt tank with %d pid. Nothing will be respawned.", tankPid);
        return nullptr;
    }

    // TODO: marty - remove tank

    // Remove tank from sets
    tanksByPosition.erase(*iter);
    Team t = (*iter)->team;
    delete *iter;
    tanksByPid.erase(iter);

    return createTank(t);
}
