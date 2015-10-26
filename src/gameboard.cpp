//
// Created by med on 23.10.15.
//

#include "gameboard.h"

Gameboard::Gameboard(int x, int y) : x(x), y(y), redKilled(0), greenKilled(0){
    if(x < 0 || y < 0){
        //cause panic
    }
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1));
    refresh();

}

int Gameboard::printframe(int x, int y) {
    for(int curX  = 0; curX <= x+1; curX++){
        for(int curY = 0; curY <= y+1; curY++){
            if(curX == 0 || curX == x+1 || curY == 0 || curY == y+1){
                mvaddch(curX, curY, '$');
                move(0,0);
            }
        }
    }
    mvaddstr(y+3, 0, "Destroyed tanks: ");
    attron(COLOR_PAIR(2));
    mvaddch(y+4, 0, '0');

    attron(COLOR_PAIR(3));
    mvaddch(y+5, 0, '0');
    refresh();
    return 0;
}

int Gameboard::insertTank(TankBean * tank) {
    if(tank->team == GREEN){
        attron(COLOR_PAIR(2));
    }
    else{
        attron(COLOR_PAIR(3));
    }
    move(tank->position.first+1, tank->position.second+1);
    addch('X');
    move(0,0);
    refresh();
    return 0;
}

int Gameboard::deleteTank(TankBean * tank) {
    move(tank->position.first+1, tank->position.second+1);
    addch(' ');
    if(tank->team == RED){
        redKilled++;
        char killed [8];
        sprintf(killed, "%d", redKilled);
        attron(COLOR_PAIR(3));
        mvaddstr(y+5, 0, killed);
    }
    else{
        greenKilled++;
        char killed [8];
        sprintf(killed, "%d", greenKilled);
        attron(COLOR_PAIR(2));
        mvaddstr(y+4, 0, killed);
    }
    move(0,0);
    refresh();
    return 0;
}

