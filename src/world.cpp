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
#include <map>
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

    setListenSocket();
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
    usleep(roundTime);
    waitForAllTanks();
}

void World::performRound()
{
    roundCount++;
    syslog(LOG_INFO, "round num %d started", roundCount);
    receiveMessages();
    performActions();
    printGameBoard();
    usleep(roundTime);
}

void World::clearTanks()
{
    for (auto row : mapToTank) {
        for (auto tank : row.second) {
            tank.second->markAsDestroyed();
        }
    }

    Tank::notifyAllTanks();

    for (Tank* t : tanks)
        delete t;

    tanks.clear();
    freeTanks.clear();
    addrToTank.clear();
    mapToTank.clear();
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
    auto &row = mapToTank[abs(rand() % areaY)];
    while (row.size() - areaX - 1 == 0) {
        row = mapToTank[abs(rand() % areaY)];
    }

    // Find random X
    while (! (row.insert(pair<int, Tank *>(abs(rand() % areaX), newTank))).second) { }

    freeTanks.push_back(newTank);
    tanks.push_back(newTank);
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
        throw runtime_error("getaddrinfo() failed");
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

void World::receiveMessages()
{
    struct sockaddr_storage from;
    char buf[2];
    socklen_t fromlen = sizeof from;
    while ((recvfrom(sd_listen, buf, 2, MSG_DONTWAIT, (struct sockaddr*)&from, &fromlen)) != -1) {
        struct sockaddr_in addr = *(struct sockaddr_in*)&from;
        Tank* tank = addrToTank[addr];

        if (tank == nullptr) {

            /* assign tank */
            while (freeTanks.size() > 0) {
                tank = freeTanks.back();
                freeTanks.pop_back();
                if (!tank->isDestroyed())
                    break;
            }

            if (tank == nullptr || tank->isDestroyed()) {
                syslog(LOG_INFO, "no more tanks for clients");
                continue;
            }

            tank->setSocket((struct sockaddr*)&from, fromlen);
            addrToTank[addr] = tank;
            tank->setNextAction(buf);
            if (sendto(sd_listen, buf, 2, 0, (struct sockaddr*)&from, fromlen) == -1) {
                syslog(LOG_ERR, "sendto() failed: %s", strerror(errno));
            }

        } else if (tank->isDestroyed()) {
            continue;

        } else {
            tank->setNextAction(buf);
            if (sendto(sd_listen, buf, 2, 0, (struct sockaddr*)&from, fromlen) == -1) {
                syslog(LOG_ERR, "sendto() failed: %s", strerror(errno));
            }
        }
    }

    if (errno != EWOULDBLOCK && errno != EAGAIN) {
        syslog(LOG_ERR, "recvfrom() failed: %s", strerror(errno));
        throw runtime_error("recvfrom() failed");
    }
}

int World::printGameBoard()
{
    syslog(LOG_INFO, "printing roung");
    // TODO: check for errors
    char comma = ',';
    char green = 'g';
    char red = 'r';
    char noTank = '0';

    namedPipe << areaX << comma << areaY << comma;

    for (int i = 0; i < areaY; ++i) {
        for (int j = 0; j < areaX; ++j) {
            if (mapToTank.find(i) != mapToTank.end() && mapToTank[i].find(j) != mapToTank[i].end()) {
                if (mapToTank[i][j]->getTeam() == GREEN)
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
    Tank::notifyAllTanks();

    // Handle FIRE action
    for (auto rowIter = mapToTank.begin(); rowIter != mapToTank.end(); ++rowIter) {
        for (auto colIter = rowIter->second.begin(); colIter != rowIter->second.end(); ++colIter) {
            Tank *tank = colIter->second;

            if (tank->waitForTank() == 0) {

                switch (tank->getAction()) {

                    case FIRE_UP: {
                        auto iterUp = mapToTank.begin();
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
                        while (++iterDown != mapToTank.end()) {
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
    auto rowIter = mapToTank.begin();
    while (rowIter != mapToTank.end()) {

        auto colIter = rowIter->second.begin();
        while (colIter != rowIter->second.end()) {
            Tank *tank = colIter->second;

            if (tank->isDestroyed()) {
                rowIter->second.erase(colIter++);
                continue;
            }

            switch (tank->getAction()) {

                case MOVE_UP:
                    // Am I at the end of map?
                    if (rowIter->first == 0) {
                        logTankRolledOffTheMap(colIter->first, rowIter->first);
                        tank->markAsDestroyed();
                    }
                    else {
                        // No tank above, Move up
                        if (rowIter == mapToTank.begin()) {
                            mapToTank[rowIter->first - 1].insert(pair<int, Tank *>(colIter->first, tank));
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
                            closestUp->second->markAsDestroyed();
                            closestUpRow->second.erase(closestUp);
                            tank->markAsDestroyed();
                        }
                        // Move up
                        else {
                            // Missing row
                            if (rowIter->first - 1 != closestUpRow->first) {
                                mapToTank[rowIter->first - 1].insert(pair<int, Tank *>(colIter->first, tank));
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
                        tank->markAsDestroyed();
                    }
                    else {
                        auto closestDownRow = rowIter;
                        closestDownRow++;

                        // Create new row and Move Down
                        if (closestDownRow == mapToTank.end() || rowIter->first + 1 != closestDownRow->first) {
                            mapToTank[rowIter->first + 1].insert(pair<int, Tank *>(colIter->first, tank));
                            tank->_setActionToUndefined();
                        }
                        // Row exist
                        else {
                            auto closestDown = closestDownRow->second.find(colIter->first);

                            // Crash
                            if (closestDown != closestDownRow->second.end()) {
                                logTankCrash(colIter->first, rowIter->first, colIter->first, closestDownRow->first);
                                closestDown->second->markAsDestroyed();
                                closestDownRow->second.erase(closestDown);
                                tank->markAsDestroyed();
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
                        tank->markAsDestroyed();
                    }
                    else {
                        auto closestRight = colIter;
                        closestRight++;

                        // Tank crash
                        if (closestRight != rowIter->second.end() && colIter->first + 1 == closestRight->first) {
                            logTankCrash(colIter->first, rowIter->first, closestRight->first, rowIter->first);
                            closestRight->second->markAsDestroyed();
                            rowIter->second.erase(closestRight);
                            tank->markAsDestroyed();
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
                        tank->markAsDestroyed();
                    }
                    else {
                        auto closestLeft = colIter;

                        // Tank crash
                        if (colIter != rowIter->second.begin() && colIter->first - 1 == (--closestLeft)->first) {
                            logTankCrash(colIter->first, rowIter->first, closestLeft->first, rowIter->first);
                            closestLeft->second->markAsDestroyed();
                            rowIter->second.erase(closestLeft);
                            tank->markAsDestroyed();
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
    for (auto rowIter = mapToTank.begin(); rowIter != mapToTank.end(); ++rowIter) {
        for (auto colIter = rowIter->second.begin(); colIter != rowIter->second.end(); ++colIter) {
            colIter->second->waitForTank();
        }
    }
}
