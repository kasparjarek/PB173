#ifndef INTERNET_OF_TANKS_GAMEBOARD_H
#define INTERNET_OF_TANKS_GAMEBOARD_H

#include <unistd.h>
#include <ncurses.h>
#include <stdlib.h>
#include <utility>
#include "tankbean.h"


class Gameboard
{
public:
    Gameboard(int x = 50,
              int y = 50);

    int printframe(int x, int y);

    int insertTank(TankBean *tank);

    int deleteTank(TankBean *tank);

    virtual ~Gameboard() {
        endwin();
    }

private:
    int x;
    int y;
};



#endif //INTERNET_OF_TANKS_GAMEBOARD_H
