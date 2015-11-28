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
                                      destroyedGreenTanks(0),
                                      destroyedRedTanks(0),
                                      greenTanks(0),
                                      redTanks(0),
                                      gameboard(nullptr)
{

    //if pipe doesn't exist, create it
    if (access(path, F_OK) != 0) {
        if (mkfifo(path, S_IRUSR | S_IWUSR) != 0) {
            syslog(LOG_ERR, "mkfifo() with name %s failed: %s",path, strerror(errno));
            throw std::runtime_error("mkfifo failed");
        }
    }

    // open pipe to world
    ifs.open (path, ifstream::in);
    if(!ifs.is_open() || !ifs.good()){
        syslog(LOG_ERR, "couldn't read x,y from pipe arg");
        throw runtime_error("couldn't read x,y from pipe");
    }

    //load size of gameboard
    readGameBoardSize();
}

int WorldClient::readGameBoardSize() {
    char coordX[16];
    char coordY[16];
    ifs.getline(coordX, 16, ',');
    ifs.getline(coordY, 16, ',');
    x = atoi(coordX);
    y = atoi(coordY);
    if(x == 0 || y == 0){
        syslog(LOG_WARNING, "x or y is zero, possible error in reading file");
    }
    return 0;
}

int WorldClient::initGameboard()
{
    //start ncurses
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    start_color();
    noecho();

    //prepare color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    gameboard = newwin(y+10, (x > x+2), 0, 0);
    wattron(gameboard, COLOR_PAIR(1));

    //print game frame
    for(int curY = 0; curY <= y+1; curY++){
        for(int curX  = 0; curX <= x+1; curX++){
            if(curX == 0 || curX == x+1 || curY == 0 || curY == y+1){
                mvwaddch(gameboard, curY, curX, '$');
                wmove(gameboard, 0,0);
            }
        }
    }
    return 0;
}

int WorldClient::terminate()
{
    ifs.close();
    wrefresh(gameboard);
    delwin(gameboard);
    endwin();
    exit(0);
}

//send signal to world process
int WorldClient::signalWorld(int signal)
{
    pid_t pid = 0;
    ifstream s(WORLD_PATH);
    if(!s.is_open()){
        syslog(LOG_INFO, "couldn't open %s, trying working dir.", WORLD_PATH);
        s.open("world.pid");
    }
    s >> pid;
    syslog(LOG_ERR, "Read world pid as %d", pid);
    if(pid){
        kill(pid, signal);
        return 0;
    }
    syslog(LOG_ERR, "couldn't read world pid");
    return -1;
}

char WorldClient::readFieldFromPipe()
{
    char field[2];
    field[0] = -1;
    ifs.getline(field, 2, ',');
    if(field[0] == -1){
        syslog(LOG_WARNING, "couldn't read field from pipe");
    }
    if(field[0] == 'r'){
        redTanks++;
    }
    if(field[0] == 'g'){
        greenTanks++;
    }
    return field[0];
}

int main(int argc, char ** argv)
{
    //handle main arguments
    char * pipe = NULL;
    char opt;
    while ((opt = (char) getopt_long(argc, argv, "p:h", LONG_ARGS, NULL)) != -1) {
        switch (opt) {
            case 'p': //pipe
                pipe = optarg;
                break;
            case 'h': //pipe
                printHelp();
                exit(0);
                break;
            default:
                printHelp();
                exit(1);
        }
    }

    WorldClient wc (pipe);
    wc.initGameboard();

    int input = 0;
    nodelay(wc.getGameboard(), true); //hopefully, this will make getchars in gameboard non-blocking

    while(input != 'q')
    {
        char tank;
        int fieldCounter = 0;
        //reload full gameboard from pipe
        while(fieldCounter < wc.getX()*wc.getY())
        {
            tank = wc.readFieldFromPipe();
            if(tank == -1 || !wc.getIfs().good()){
                break;
            }
            switch (tank) {
                case '0':
                    mvwaddch(wc.getGameboard(), fieldCounter / wc.getX()+1, fieldCounter % wc.getX()+1, ' ');
                    fieldCounter++;
                    break;
                case 'g':
                    wattron(wc.getGameboard(), COLOR_PAIR(2));
                    mvwaddch(wc.getGameboard(), fieldCounter / wc.getX()+1, fieldCounter % wc.getX()+1, 'X');
                    fieldCounter++;
                    wc.setGreenTanks(wc.getGreenTanks() + 1);
                    break;
                case 'r':
                    wattron(wc.getGameboard(), COLOR_PAIR(3));
                    mvwaddch(wc.getGameboard(), fieldCounter / wc.getX()+1, fieldCounter % wc.getX()+1, 'X');
                    fieldCounter++;
                    wc.setRedTanks(wc.getRedTanks() + 1);
                    break;
                default:break;
            }
        }

        //check, if there is input waiting
        input = wgetch(wc.getGameboard());
        switch (input){
            case 'q':
                wc.terminate();
            case 'x':
                wc.signalWorld(SIGINT);
                break;
            case 'r':
                wc.signalWorld(SIGUSR1);
            default: //in case of none or unknown input do nothing
                break;
        }
    }
    wc.terminate();
    return 0;
}