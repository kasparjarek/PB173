//
// Created by med on 30.10.15.
//

#ifndef INTERNET_OF_TANKS_WORLDCLIENT_H
#define INTERNET_OF_TANKS_WORLDCLIENT_H

#include <ncurses.h>
#include <iosfwd>
#include <fstream>

#define WORLD_PATH "/var/run/world.pid"
#define FIELD_EMPTY '0'
#define FIELD_TANK 'X'

class WorldClient {
private:
    int y;
    int x;
    int destroyedGreenTanks;
    int destroyedRedTanks;
    int greenTanks;
    int redTanks;
    WINDOW * gameboard;
    std::ifstream ifs;
public:
    WorldClient(char *);

    WINDOW *getGameboard() const {
        return gameboard;
    }

/**
     * Init gameboard - starts ncurses, print game frame and statistics
     */
    int initGameboard();

    /**
     * close ncurses, exit
     */
    int terminate();

/**
     * send signal to world process
     * @param signal number
     */
    int signalWorld(int);

    /**
     * reads one field from pipe file stream
     * @return -1 on error, 0
     */
    char readFieldFromPipe();

    int getX() const {
        return x;
    }

    int getY() const {
        return y;
    }

    void setDestroyedRedTanks(int destroyedRedTanks) {
        WorldClient::destroyedRedTanks = destroyedRedTanks;
    }

    void setDestroyedGreenTanks(int destroyedGreenTanks) {
        WorldClient::destroyedGreenTanks = destroyedGreenTanks;
    }

    int getDestroyedGreenTanks() const {
        return destroyedGreenTanks;
    }

    int getDestroyedRedTanks() const {
        return destroyedRedTanks;
    }

    int getRedTanks() const {
        return redTanks;
    }

    int getGreenTanks() const {
        return greenTanks;
    }

    const std::ifstream &getIfs() const {
        return ifs;
    }
    void setRedTanks(int red_tanks) {
        WorldClient::redTanks = red_tanks;
    }

    void setGreenTanks(int green_tanks) {
        WorldClient::greenTanks = green_tanks;
    }
};


#endif //INTERNET_OF_TANKS_WORLDCLIENT_H
