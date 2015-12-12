#include "world.h"
#include "tank.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <syslog.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

using std::runtime_error;
using std::pair;

const char* IOT_PORT = "1337";

World::World(int areaX,
             int areaY,
             int redCount,
             int greenCount,
             std::string & namedPipe,
             useconds_t roundTime)
    : areaX(areaX), areaY(areaY), redCount(redCount), greenCount(greenCount),
      roundTime(roundTime), roundCount(0)
{
    srand((unsigned int) time(NULL));

    this->namedPipe.open(namedPipe);

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

    setListenSocket();

    printGameBoard();
    sleep(roundTime);
}

void World::performRound()
{
    syslog(LOG_INFO, "round %d starting", roundCount);
    roundCount++;
    syslog(LOG_INFO, "round num %d started", roundCount);
    performActions();
    printGameBoard();
    sleep(roundTime);
}

void World::clearTanks()
{
    waitForAllTanks();
    Tank::notifyAllTanks();
    waitForAllTanks();

    for (auto row : tanks) {
        for (auto tank : row.second) {
            tank.second->markAsDestroyed();
        }
    }

    Tank::notifyAllTanks();

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
    while (! (row.insert(pair<int, Tank *>(abs(rand() % areaX), newTank))).second) { }

    return newTank;
}

void World::createTanks(Team team, int count)
{
    for (int i = 0; i < count; ++i) {
        createTank(team);
    }
}

void World::setListenSocket()
{
    struct addrinfo hints;
    struct addrinfo* server_info;
    struct addrinfo* p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, IOT_PORT, &hints, &server_info)) != 0) {
        syslog(LOG_ERR, "getaddrinfo() failed: %s", gai_strerror(status));
        exit(1);
    }

    for (p = server_info; p != NULL; p = p->ai_next) {
        if ((sd_listen = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol)) == -1) {
            syslog(LOG_INFO, "socket() failed: %s", strerror(errno));
            continue;
        }

        if (bind(sd_listen, p->ai_addr, p->ai_addrlen) == -1) {
            close(sd_listen);
            syslog(LOG_INFO, "bind() failed: %s", strerror(errno));
            continue;
        }

        break;
    }

    if (p == NULL) {
        syslog(LOG_ERR, "failed to bind socket");
        freeaddrinfo(server_info);
        throw runtime_error("failed to bind socket");
    }

    freeaddrinfo(server_info);
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

    namedPipe.flush();
    return 0;
}

int World::performActions()
{
    std::vector<Tank *> destroyedTanks;

    /* Wait for tanks to be ready for action */
    waitForAllTanks();
    Tank::notifyAllTanks();

    // Handle FIRE action
    for (auto rowIter = tanks.begin(); rowIter != tanks.end(); ++rowIter) {
        for (auto colIter = rowIter->second.begin(); colIter != rowIter->second.end(); ++colIter) {
            Tank *tank = colIter->second;

            if (tank->waitForTank() == 0) {

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
                destroyedTanks.push_back(tank);
                rowIter->second.erase(colIter++);
                continue;
            }

            switch (tank->getAction()) {

                case MOVE_UP:
                    // Am I at the end of map?
                    if (rowIter->first == 0) {
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
                        destroyedTanks.push_back(tank->markAsDestroyed());
                    }
                    else {
                        // No tank above, Move up
                        if (rowIter == tanks.begin()) {
                            tanks[rowIter->first - 1].insert(pair<int, Tank *>(colIter->first, tank));
                            rowIter->second.erase(colIter++);
                            tank->_setActionToUndefined();
                            continue;
                        }

                        auto closestUpRow = rowIter;
                        closestUpRow--;
                        auto closestUp = closestUpRow->second.find(colIter->first);

                        // Tank crash
                        if (rowIter->first - 1 == closestUpRow->first && closestUp != closestUpRow->second.end()) {
                            logTankCrash(colIter->first, rowIter->first, colIter->first, closestUpRow->first);
                            destroyedTanks.push_back(closestUp->second->markAsDestroyed());
                            closestUpRow->second.erase(closestUp);
                            destroyedTanks.push_back(tank->markAsDestroyed());
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
                    rowIter->second.erase(colIter++);
                    continue;


                case MOVE_DOWN:
                    // Am I at the end of map?
                    if (rowIter->first + 1 >= areaY) {
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
                        destroyedTanks.push_back(tank->markAsDestroyed());
                    }
                    else {
                        auto closestDownRow = rowIter;
                        closestDownRow++;

                        // Create new row and Move Down
                        if (closestDownRow == tanks.end() || rowIter->first + 1 != closestDownRow->first) {
                            tanks[rowIter->first + 1].insert(pair<int, Tank *>(colIter->first, tank));
                            tank->_setActionToUndefined();
                        }
                        // Row exist
                        else {
                            auto closestDown = closestDownRow->second.find(colIter->first);

                            // Crash
                            if (closestDown != closestDownRow->second.end()) {
                                logTankCrash(colIter->first, rowIter->first, colIter->first, closestDownRow->first);
                                destroyedTanks.push_back(closestDown->second->markAsDestroyed());
                                closestDownRow->second.erase(closestDown);
                                destroyedTanks.push_back(tank->markAsDestroyed());
                            }
                            // Move down
                            else {
                                closestDownRow->second.insert(pair<int, Tank *>(colIter->first, tank));
                                tank->_setActionToUndefined();
                            }
                        }
                    }
                    rowIter->second.erase(colIter++);
                    continue;


                case MOVE_RIGHT:
                    // Am I at the end of map?
                    if (colIter->first + 1 >= areaX) {
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
                        destroyedTanks.push_back(tank->markAsDestroyed());
                    }
                    else {
                        auto closestRight = colIter;
                        closestRight++;

                        // Tank crash
                        if (closestRight != rowIter->second.end() && colIter->first + 1 == closestRight->first) {
                            logTankCrash(colIter->first, rowIter->first, closestRight->first, rowIter->first);
                            destroyedTanks.push_back(closestRight->second->markAsDestroyed());
                            rowIter->second.erase(closestRight);
                            destroyedTanks.push_back(tank->markAsDestroyed());
                        }
                        // Move Right
                        else {
                            rowIter->second.insert(colIter, pair<int, Tank *>(colIter->first + 1, tank));
                            tank->_setActionToUndefined();
                        }

                    }
                    rowIter->second.erase(colIter++);
                    continue;


                case MOVE_LEFT:
                    // Am I at the end of map?
                    if (colIter->first == 0) {
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
                        destroyedTanks.push_back(tank->markAsDestroyed());
                    }
                    else {
                        auto closestLeft = colIter;

                        // Tank crash
                        if (colIter != rowIter->second.begin() && colIter->first - 1 == (--closestLeft)->first) {
                            logTankCrash(colIter->first, rowIter->first, closestLeft->first, rowIter->first);
                            destroyedTanks.push_back(closestLeft->second->markAsDestroyed());
                            rowIter->second.erase(closestLeft);
                            destroyedTanks.push_back(tank->markAsDestroyed());
                        }
                        // Move Left
                        else {
                            rowIter->second.insert(pair<int, Tank *>(colIter->first - 1, tank));
                            tank->_setActionToUndefined();
                        }
                    }
                    rowIter->second.erase(colIter++);
                    continue;


                default:
                    break;
            }


            colIter++;
        }

        rowIter++;
    }

    // delete all destroyed tanks
    Tank::notifyAllTanks();

    for (Tank *i : destroyedTanks) {
        delete i;
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

void World::waitForAllTanks()
{
    for (auto rowIter = tanks.begin(); rowIter != tanks.end(); ++rowIter) {
        for (auto colIter = rowIter->second.begin(); colIter != rowIter->second.end(); ++colIter) {
            colIter->second->waitForTank();
        }
    }
}
