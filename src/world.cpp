#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/errno.h>
#include "world.h"
#include "tank.h"

using namespace std;

World::World(int areaX,
             int areaY,
             int redCount,
             int greenCount,
             int namedPipe,
             useconds_t roundTime,
             const char *const greenTankPath,
             const char *const redTankPath)
    : areaX(areaX), areaY(areaY), redCount(redCount), greenCount(greenCount), namedPipe(namedPipe),
      roundTime(roundTime), greenTankPath(greenTankPath), redTankPath(redTankPath), roundCount(0)
{
    srand((unsigned int) time(NULL));

    if (areaX < 0 || areaY < 0 || redCount < 0 || greenCount < 0 || (areaY * areaX < redCount + greenCount)) {
        throw runtime_error("invalid parameters");
    }
}

int World::init()
{
    if (createTanks(Team::GREEN, greenCount) != 0) {
        return -1;
    }

    if (createTanks(Team::RED, redCount) != 0) {
        return -1;
    }

    return 0;
}

void World::performRound()
{
    roundCount++;
    performActions();
    printGameBoard();
    usleep(roundTime);
}

int World::restart()
{
    clearTanks();
    roundCount = 0;
    return init();
}

void World::clearTanks()
{
    for (auto row : tanks) {
        for (auto tank : row.second) {
            delete tank.second;
        }
    }

    tanks.clear();
}

Tank *World::createTank(Team team)
{
    Tank *newTank = nullptr;

    try {
        newTank = new Tank(team, (team == Team::GREEN) ? greenTankPath : redTankPath);
    }
    catch (runtime_error error) {
        syslog(LOG_ERR, "Creating new tank failed: %s", error.what());
        return nullptr;
    }

    // Find random Y
    auto row = tanks[abs(rand() % areaY)];
    while (row.size() - areaX - 1 == 0) {
        row = tanks[abs(rand() % areaY)];
    }

    // Find random X
    while (!row.insert(pair<int, Tank *>(abs(rand() % areaX), newTank)).second) {

    }

    return newTank;
}

int World::createTanks(Team team, int count)
{
    for (int i = 0; i < count; ++i) {
        if (createTank(team) == nullptr) {
            clearTanks();
            return -1;
        }
    }
    return 0;
}

int World::printGameBoard()
{
    char comma = ',';
    char green = 'g';
    char red = 'r';
    char notank = '0';

    write(namedPipe, &areaX, sizeof(areaX));
    write(namedPipe, &comma, sizeof(char));
    write(namedPipe, &areaY, sizeof(areaY));
    write(namedPipe, &comma, sizeof(char));

    for (int i = 0; i < areaY; ++i) {
        for (int j = 0; j < areaX; ++j) {
            if (tanks.find(i) != tanks.end() && tanks[i].find(j) != tanks[i].end()) {
                if (tanks[i][j]->getTeam() == GREEN)
                    write(namedPipe, &green, sizeof(char));
                else
                    write(namedPipe, &red, sizeof(char));
            } else {
                write(namedPipe, &notank, sizeof(char));
            }
            write(namedPipe, &comma, sizeof(char));
        }
    }

    return 0;
}

int World::performActions()
{
    // Handle FIRE action
    for (auto rowIter = tanks.begin(); rowIter != tanks.end(); ++rowIter) {
        for (auto colIter = rowIter->second.begin(); colIter != rowIter->second.end(); ++colIter) {
            Tank *tank = colIter->second;

            if (tank->fetchAction() == 0) {

                switch (tank->getAction()) {

                    case FIRE_UP: {
                        auto iterUp = tanks.begin();
                        while (iterUp != rowIter) {
                            auto tankIterUp = iterUp->second.find(colIter->first);
                            if (tankIterUp != iterUp->second.end()) {
                                logTankHit(colIter->second->getPid(), colIter->first, rowIter->first,
                                           tankIterUp->second->getPid(), colIter->first, iterUp->first);
                                tankIterUp->second->markAsDestroyed();
                            }
                            iterUp++;
                        }
                        break;
                    }

                    case FIRE_DOWN: {
                        auto iterDown = rowIter;
                        while (++iterDown != tanks.end()) {
                            auto tankIterDown = iterDown->second.find(colIter->first);
                            if (tankIterDown != iterDown->second.end()) {
                                logTankHit(colIter->second->getPid(), colIter->first, rowIter->first,
                                           tankIterDown->second->getPid(), colIter->first, iterDown->first);
                                tankIterDown->second->markAsDestroyed();
                            }
                        }
                        break;
                    }

                    case FIRE_RIGHT: {
                        auto iterRight = colIter;
                        while (++iterRight != rowIter->second.end()) {
                            logTankHit(colIter->second->getPid(), colIter->first, rowIter->first,
                                       iterRight->second->getPid(), iterRight->first, rowIter->first);
                            iterRight->second->markAsDestroyed();
                        }
                        break;
                    }

                    case FIRE_LEFT: {
                        auto iterLeft = rowIter->second.begin();
                        while (iterLeft != colIter) {
                            logTankHit(colIter->second->getPid(), colIter->first, rowIter->first,
                                       iterLeft->second->getPid(), iterLeft->first, rowIter->first);
                            iterLeft->second->markAsDestroyed();
                            iterLeft++;
                        }
                        break;
                    }

                    default:
                        break;
                }
            }
        }
    }

    // Handle MOVE action and remove destroyed tanks
    auto rowIter = tanks.begin();
    while (rowIter != tanks.end()) {

        auto colIter = rowIter->second.begin();
        while (colIter != rowIter->second.end()) {
            Tank *tank = colIter->second;

            if (tank->isDestroyed()) {
                rowIter->second.erase(colIter++);
                delete tank;
                continue;
            }

            switch (tank->getAction()) {

                case MOVE_UP:
                    // Am I at the end of map?
                    if (rowIter->first == 0) {
                        logTankRolledOffTheMap(tank->getPid(), colIter->first, rowIter->first);
                    }
                    else {
                        // No tank above, Move up
                        if (rowIter == tanks.begin()) {
                            tanks[rowIter->first - 1].insert(pair<int, Tank *>(colIter->first, tank));
                            rowIter->second.erase(colIter++);
                            continue;
                        }

                        auto closestUpRow = rowIter;
                        closestUpRow--;
                        auto closestUp = closestUpRow->second.find(colIter->first);

                        // Tank crash
                        if (rowIter->first - 1 == closestUpRow->first && closestUp != closestUpRow->second.end()) {
                            logTankCrash(tank->getPid(), colIter->first, rowIter->first,
                                         closestUp->second->getPid(), colIter->first, closestUpRow->first);
                            delete closestUp->second;
                            closestUpRow->second.erase(closestUp);
                        }
                        // Move up
                        else {
                            // Missing row
                            if (rowIter->first - 1 != closestUpRow->first) {
                                tanks[rowIter->first - 1].insert(pair<int, Tank *>(colIter->first, tank));
                            }
                            else {
                                closestUpRow->second.insert(pair<int, Tank *>(colIter->first, tank));
                            }
                        }
                    }
                    delete tank;
                    rowIter->second.erase(colIter++);
                    continue;


                case MOVE_DOWN:
                    // Am I at the end of map?
                    if (rowIter->first + 1 >= areaY) {
                        logTankRolledOffTheMap(tank->getPid(), colIter->first, rowIter->first);
                    }
                    else {
                        auto closestDownRow = rowIter;
                        closestDownRow++;

                        // Create new row and Move Down
                        if (closestDownRow == tanks.end() || rowIter->first + 1 != closestDownRow->first) {
                            tanks[rowIter->first + 1].insert(pair<int, Tank *>(colIter->first, tank));
                        }
                        // Row exist
                        else {
                            auto closestDown = closestDownRow->second.find(colIter->first);

                            // Crash
                            if (closestDown != closestDownRow->second.end()) {
                                logTankCrash(tank->getPid(), colIter->first, rowIter->first,
                                             closestDown->second->getPid(), colIter->first, closestDownRow->first);
                                delete closestDown->second;
                                closestDownRow->second.erase(closestDown);
                            }
                            // Move down
                            else {
                                closestDownRow->second.insert(pair<int, Tank *>(colIter->first, tank));
                            }
                        }
                    }
                    delete tank;
                    rowIter->second.erase(colIter++);
                    continue;


                case MOVE_RIGHT:
                    // Am I at the end of map?
                    if (colIter->first + 1 >= areaX) {
                        logTankRolledOffTheMap(tank->getPid(), colIter->first, rowIter->first);
                    }
                    else {
                        auto closestRight = colIter;
                        closestRight++;

                        // Tank crash
                        if (closestRight != rowIter->second.end() && colIter->first + 1 == closestRight->first) {
                            logTankCrash(tank->getPid(), colIter->first, rowIter->first,
                                         closestRight->second->getPid(), closestRight->first, rowIter->first);
                            delete closestRight->second;
                            rowIter->second.erase(closestRight);
                        }
                        // Move Right
                        else {
                            rowIter->second.insert(colIter, pair<int, Tank *>(colIter->first + 1, tank));
                        }

                    }
                    delete tank;
                    rowIter->second.erase(colIter++);
                    continue;


                case MOVE_LEFT:
                    // Am I at the end of map?
                    if (colIter->first == 0) {
                        logTankRolledOffTheMap(tank->getPid(), colIter->first, rowIter->first);
                    }
                    else {
                        auto closestLeft = colIter;
                        closestLeft--;

                        // Tank crash
                        if (colIter != rowIter->second.begin() && colIter->first - 1 == closestLeft->first) {
                            logTankCrash(tank->getPid(), colIter->first, rowIter->first,
                                         closestLeft->second->getPid(), closestLeft->first, rowIter->first);
                            delete closestLeft->second;
                            rowIter->second.erase(closestLeft);
                        }
                        // Move Left
                        else {
                            rowIter->second.insert(pair<int, Tank *>(colIter->first - 1, tank));
                        }
                    }
                    delete tank;
                    rowIter->second.erase(colIter++);
                    continue;


                default:
                    break;
            }


            colIter++;
        }

        rowIter++;
    }

    return 0;
}
void World::logTankHit(pid_t aggressorPid, int aggressorX, int aggressorY, pid_t victimPid, int victimX, int victimY)
{
    syslog(LOG_INFO, "Aggresor with pid %d at [%d,%d] destroy tank with pid %d on [%d,%d].",
           aggressorPid, aggressorX, aggressorY, victimPid, victimX, victimY);
}
void World::logTankRolledOffTheMap(pid_t pid, int x, int y)
{
    syslog(LOG_INFO, "Tank with pid %d at [%d,%d] rolled off the map.", pid, x, y);
}
void World::logTankCrash(pid_t aggressorPid, int aggressorX, int aggressorY, pid_t victimPid, int victimX, int victimY)
{
    syslog(LOG_INFO, "Tank with pid %d at [%d,%d] crashed into tank with pid %d at [%d,%d].",
           aggressorPid, aggressorX, aggressorY, victimPid, victimX, victimY);
}
