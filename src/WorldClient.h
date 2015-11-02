//
// Created by med on 30.10.15.
//

#ifndef INTERNET_OF_TANKS_WORLDCLIENT_H
#define INTERNET_OF_TANKS_WORLDCLIENT_H

#define WORLD_PATH "/var/run/world.pid"

#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/syslog.h>
#include <signal.h>
#include <stdexcept>
#include <fcntl.h>
#include <ncurses.h>
#include <iostream>
#include <string.h>
#include <getopt.h>
#include <ncurses.h>

class WorldClient {
private:
    int y;
    int x;
    int destroyed_green_tanks;
    int destroyed_red_tanks;
    int green_tanks;
    int red_tanks;
    int pipe_input;
    int coord_offset;
public:
    WorldClient(char *);

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
    int terminateWorld(int);

    /**
     * set offset of data containing game fields
     */
    int getBoardDataStart(char *);


    int getX() const {
        return x;
    }

    int getY() const {
        return y;
    }

    int getPipe_input() const {
        return pipe_input;
    }

    size_t getCoord_offset() const {
        return coord_offset;
    }
};


#endif //INTERNET_OF_TANKS_WORLDCLIENT_H
