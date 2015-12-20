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
    std::string pipeName;

    /**
     * Send signal to world process
     * @param signal number
     */
    int signalWorld(int);

    /**
     * Wait until pipe become ready for reading with timeout
     * @return 0 if pipe is ready and -1 if timeout expires
     */
    int checkIfPipeIsReady();

    /**
     * Reads two integers from current position in ifs to x and y
     */
    int readGameBoardSize();

    /**
     * Reads one field from pipe file stream
     * @return -1 on error, 0
     */
    char readFieldFromPipe();

    /**
     * Init gameboard - starts ncurses, print game frame and statistics
     */
    int printGameboardFrame();

public:

    WorldClient(char *);

    virtual ~WorldClient()
    {
        unlink(pipeName.c_str());
        endwin();
    }

    /**
     * Print gameboard from pipe
     * @return 0 if gameboard was printed and -1 if timeout expires and nothing was printed
     */
    int printGameboard();

    /**
     * Non-blocking read input from user.
     * @return -1 if user press terminating command and app should exit
     */
    int handleInput();

};


#endif //INTERNET_OF_TANKS_WORLDCLIENT_H
