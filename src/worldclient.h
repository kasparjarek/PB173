//
// Created by med on 30.10.15.
//

#ifndef INTERNET_OF_TANKS_WORLDCLIENT_H
#define INTERNET_OF_TANKS_WORLDCLIENT_H

#include <ncurses.h>
#include <iosfwd>
#include <fstream>

#define WORLD_PATH "world.pid"

class WorldClient {
private:
    int y;
    int x;
    int pipe;
    WINDOW * gameboard;
public:
    WorldClient(char *);

    WINDOW *getGameboard() const {
        return gameboard;
    }

    /**
     * Reads two integers from current position in ifs to x and y
     */
    int readGameBoardSize();

    /**
     * Init gameboard - starts ncurses, print game frame and statistics
     */
    int initGameboard();

    /**
     * Close ncurses, exit
     */
    int terminate();

/**
     * Send signal to world process
     * @param signal number
     */
    int signalWorld(int);

    /**
     * Reads one field from pipe file stream
     * @return -1 on error, 0
     */
    char readFieldFromPipe();

    int getX() const {
        return x;
    }

    int getY() const {
        return y;
    }
};


#endif //INTERNET_OF_TANKS_WORLDCLIENT_H
