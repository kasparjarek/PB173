#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/errno.h>
#include <system_error>
#include "world.h"
#include "tank.h"

using std::runtime_error;
using std::pair;

World::World(int areaX,
             int areaY,
             int redCount,
             int greenCount,
             std::string namedPipe,
             useconds_t roundTime)
    : areaX(areaX), areaY(areaY), redCount(redCount), greenCount(greenCount), namedPipe(namedPipe),
      roundTime(roundTime), roundCount(0)
{
    srand((unsigned int) time(NULL));

    if (areaX < 0 || areaY < 0 || redCount < 0 || greenCount < 0 || (areaY * areaX < redCount + greenCount)) {
        throw runtime_error("Creating world failed: invalid parameters");
    }
}

void World::init()
{
    clearTanks();
    roundCount = 0;
    try {
        createTanks(Team::GREEN, greenCount);
        createTanks(Team::RED, redCount);
    }
    catch (runtime_error error) {
        throw runtime_error(std::string("World initialization failed: ") + error.what());
    }
    printGameBoard();
}

void World::performRound()
{
    roundCount++;
    performActions();
    printGameBoard();
    usleep(roundTime);
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
        newTank = new Tank(team);
    }
    catch (runtime_error error) {
        syslog(LOG_ERR, "Creating new tank failed: %s", error.what());
        throw runtime_error(std::string("Creating new tank failed: ") + error.what());
    }

    // Find random Y
    auto &row = tanks[abs(rand() % areaY)];
    while (row.size() - areaX - 1 == 0) {
        row = tanks[abs(rand() % areaY)];
    }

    // Find random X
    while (!row.insert(pair<int, Tank *>(abs(rand() % areaX), newTank)).second) { }

    return newTank;
}

void World::createTanks(Team team, int count)
{
    for (int i = 0; i < count; ++i) {
        createTank(team);
    }
}

int World::printGameBoard()
{
    // TODO: check for errors
    char comma = ',';
    char green = 'g';
    char red = 'r';
    char noTank = '0';

    namedPipe << areaX << comma << areaY << comma;

    for (int i = 0; i < areaY; ++i) {
        for (int j = 0; j < areaX; ++j) {
            if (tanks.find(i) != tanks.end() && tanks[i].find(j) != tanks[i].end()) {
                if (tanks[i][j]->getTeam() == GREEN)
                    namedPipe << green;
                else
                    namedPipe << red;
            } else {
                namedPipe << noTank;
            }
            namedPipe << comma;
        }
    }

    return 0;
}

int World::performActions()
{
    Tank::requireActionsFromAllTanks();

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
                                logTankHit(colIter->first, rowIter->first, colIter->first, iterUp->first);
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
                                logTankHit(colIter->first, rowIter->first, colIter->first, iterDown->first);
                                tankIterDown->second->markAsDestroyed();
                            }
                        }
                        break;
                    }

                    case FIRE_RIGHT: {
                        auto iterRight = colIter;
                        while (++iterRight != rowIter->second.end()) {
                            logTankHit(colIter->first, rowIter->first, iterRight->first, rowIter->first);
                            iterRight->second->markAsDestroyed();
                        }
                        break;
                    }

                    case FIRE_LEFT: {
                        auto iterLeft = rowIter->second.begin();
                        while (iterLeft != colIter) {
                            logTankHit(colIter->first, rowIter->first, iterLeft->first, rowIter->first);
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
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
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
                            logTankCrash(colIter->first, rowIter->first, colIter->first, closestUpRow->first);
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
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
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
                                logTankCrash(colIter->first, rowIter->first, colIter->first, closestDownRow->first);
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
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
                    }
                    else {
                        auto closestRight = colIter;
                        closestRight++;

                        // Tank crash
                        if (closestRight != rowIter->second.end() && colIter->first + 1 == closestRight->first) {
                            logTankCrash(colIter->first, rowIter->first, closestRight->first, rowIter->first);
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
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
                    }
                    else {
                        auto closestLeft = colIter;
                        closestLeft--;

                        // Tank crash
                        if (colIter != rowIter->second.begin() && colIter->first - 1 == closestLeft->first) {
                            logTankCrash(colIter->first, rowIter->first, closestLeft->first, rowIter->first);
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

void World::logTankHit(int aggressorX, int aggressorY, int victimX, int victimY)
{
    syslog(LOG_INFO, "Aggresor at [%d,%d] destroy tank at [%d,%d].",
           aggressorX, aggressorY, victimX, victimY);
}

void World::logTankRolledOffTheMap(int x, int y)
{
    syslog(LOG_INFO, "Tank with at [%d,%d] rolled off the map.", x, y);
}

void World::logTankCrash(int aggressorX, int aggressorY, int victimX, int victimY)
{
    syslog(LOG_INFO, "Tank at [%d,%d] crashed into tank at [%d,%d].",
           aggressorX, aggressorY, victimX, victimY);
}
