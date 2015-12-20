#include "worldclient.h"

#include <unistd.h>
#include <sys/syslog.h>
#include <getopt.h>
#include <signal.h>
#include <sys/inotify.h>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

const struct option LONG_ARGS[] = {
        {"help", no_argument, NULL, 'h'},
        {"pipe", required_argument, NULL, 'p'},
        {0, 0, 0, 0}
};

void printHelp()
{
    cout << "Usage:" << endl;
    cout << "\t" << "-p, --pipe <path>" << endl;
    cout << "\t" << "path to named pipe of world program" << endl;
    cout << "\t" << "-h, --help" << endl;
}

WorldClient::WorldClient(char *path): y(0),
                                      x(0),
                                      pipe(0),
                                      pipeName(path)
{

    //if pipe doesn't exist, create it
    if (access(path, F_OK) != 0) {
        if (mkfifo(path, S_IRUSR | S_IWUSR) != 0) {
            syslog(LOG_ERR, "mkfifo() with name %s failed: %s",path, strerror(errno));
            throw std::runtime_error("mkfifo failed");
        }
    }

    // open pipe to world
    pipe = open(path, O_RDONLY);

    //start ncurses
    initscr();
    cbreak();
    start_color();
    noecho();
    curs_set(0);
    nodelay(stdscr, true);

    //prepare color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1));
}

int WorldClient::readGameBoardSize() {
    char cur;
    char buffer[16];
    memset(buffer, 0, 16);
    int counter = 0;
    while (1){
        if (read(pipe, &cur, 1) == -1) {
            //TODO: co kdyz selze + timeout
        }
        if (cur == ',')
            break;

        buffer[counter] = cur;
        counter++;
    }
    x = atoi(buffer);
    syslog(LOG_INFO, "read x from pipe: %d", x);

    memset(buffer, 0, 16);
    counter = 0;
    while (1){
        read(pipe, &cur, 1);
        if (cur == ',')
            break;

        buffer[counter] = cur;
        counter++;
    }
    y = atoi(buffer);
    syslog(LOG_INFO, "read y from pipe: %d", y);

    return 0;
}

int WorldClient::printGameboardFrame()
{
    //load size of gameboard
    readGameBoardSize();

    attron(COLOR_PAIR(1));

    //print game frame
    for (int curY = 0; curY <= y+1; curY++) {
        for (int curX  = 0; curX <= x+1; curX++) {
            if (curX == 0 || curX == x+1 || curY == 0 || curY == y+1) {
                mvaddch(curY, curX, '$');
            }
        }
    }
    return 0;
}

//send signal to world process
int WorldClient::signalWorld(int signal)
{
    pid_t pid = 0;
    ifstream s(WORLD_PATH);
    s >> pid;
    
    if (pid) {
        syslog(LOG_INFO, "Read world pid as %d and send signal %d", pid, signal);
        kill(pid, signal);
        return 0;
    }
    syslog(LOG_ERR, "Couldn't read world pid");
    return -1;
}

char WorldClient::readFieldFromPipe()
{
    char field[2];
    if (read(pipe, field, 2) == -1) {
        syslog(LOG_ERR, "couldn't ->read<- field from pipe");
    }
    if (field[1] != ',') {
        syslog(LOG_ERR, "illegal field separator from pipe");
    }
    return field[0];

}

int WorldClient::handleInput()
{
    while (int input = getch() != ERR) {
        syslog(LOG_ERR, "INSIDE WHILE read %c", input);
        switch (input)
        {
            case 'q':
                return -1;
            case 'x':
                this->signalWorld(SIGINT);
                break;
            case 'r':
                this->signalWorld(SIGUSR1);
            default: //in case of none or unknown input do nothing
                break;
        }
    }
    return 0;
}

int WorldClient::checkIfPipeIsReady()
{
    return 0;
}

int WorldClient::printGameboard()
{
    if (checkIfPipeIsReady() != 0) {
        return -1;
    }

    // Clear curses screen
    clear();

    // Print frame
    printGameboardFrame();

    // Print tanks
    for (int y = 1; y <= this->y; y++) {
        for (int x = 1; x <= this->x; x++) {
            char tank = readFieldFromPipe();

            switch (tank)
            {
                case '0':
                    mvaddch(y, x, ' ');
                    break;
                case 'g':
                    attron(COLOR_PAIR(2));
                    mvaddch(y, x, 'X');
                    break;
                case 'r':
                    attron(COLOR_PAIR(3));
                    mvaddch(y, x, 'X');
                    break;
                default:
                    syslog(LOG_ERR, "illegal field input: %c", tank);
            }
        }
    }
    return 0;
}

int main(int argc, char ** argv)
{
    //handle main arguments
    char * pipe = nullptr;
    char opt;
    while ((opt = (char) getopt_long(argc, argv, "p:h", LONG_ARGS, NULL)) != -1) {
        switch (opt)
        {
            case 'p': //pipe
                pipe = optarg;
                break;
            case 'h': //pipe
                printHelp();
                exit(0);
            default:
                printHelp();
                exit(1);
        }
    }
    if (pipe == nullptr) {
        std::cout << "Pipe arg required." << std::endl;
        syslog(LOG_ERR, "Argument pipe is required. Exitting");
        return -1;
    }

    try {
        WorldClient wc (pipe);

        while (wc.handleInput() != -1) {
            wc.printGameboard();
        }

    } catch (std::runtime_error &err){
        syslog(LOG_ERR, "caught exception: %s", err.what());
        return -1;
    }

    return 0;
}
