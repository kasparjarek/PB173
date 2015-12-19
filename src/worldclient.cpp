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
                                      x(0)
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
}

int WorldClient::readGameBoardSize() {
    char cur;
    char buffer[16];
    memset(buffer, 0, 16);
    int counter = 0;
    while(1){
        if(read(pipe, &cur, 1) == -1){
            //TODO: co kdyz selze + timeout
        }
        if(cur == ',')
            break;
        buffer[counter] = cur;
        counter++;
    }
    x = atoi(buffer);
    syslog(LOG_INFO, "read x from pipe: %d", x);

    memset(buffer, 0, 16);
    counter = 0;
    while(1){
        read(pipe, &cur, 1);
        if(cur == ',')
            break;
        buffer[counter] = cur;
        counter++;
    }
    y = atoi(buffer);
    syslog(LOG_INFO, "read y from pipe: %d", y);

    return 0;
}

int WorldClient::initGameboard()
{
    //load size of gameboard
    readGameBoardSize();

    //start ncurses
    initscr();

    if(LINES < y || COLS < x){
        syslog(LOG_ERR, "insuficient window");
        exit(1);
    }

    cbreak();
    keypad(stdscr, TRUE);
    start_color();
    noecho();
    curs_set(0);

    //prepare color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    attron(COLOR_PAIR(1));

    //print game frame
    for(int curY = 0; curY <= y+1; curY++){
        for(int curX  = 0; curX <= x+1; curX++){
            if(curX == 0 || curX == x+1 || curY == 0 || curY == y+1){
                mvaddch(curY, curX, '$');
                move(0,0);
            }
        }
    }
    return 0;
}

void WorldClient::terminate(char * p)
{
    unlink(p);
    refresh();
    endwin();
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
    syslog(LOG_WARNING, "Pid read from file: %s", std::to_string(pid).c_str());
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
    if(read(pipe, field, 2) == -1){
        syslog(LOG_ERR, "couldn't ->read<- field from pipe");
    }
    if(field[1] != ','){
        syslog(LOG_ERR, "illegal field separator from pipe");
    }
    return field[0];

}

int main(int argc, char ** argv)
{
    //handle main arguments
    char * pipe = nullptr;
    char opt;
    while ((opt = (char) getopt_long(argc, argv, "p:h", LONG_ARGS, NULL)) != -1) {
        switch (opt) {
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
    if(pipe == nullptr){
        std::cout << "Pipe arg required." << std::endl;
        syslog(LOG_ERR, "Argument pipe is required. Exitting");
        return -1;
    }


    try{
        WorldClient wc (pipe);

        wc.initGameboard();

        int input = 0;
        nodelay(stdscr, true); //hopefully, this will make getchars in gameboard non-blocking

        while(1)
        {
            char tank;
            int fieldCounter = 0;
            //reload full gameboard from pipe
            while(fieldCounter < wc.getX()*wc.getY())
            {
                tank = wc.readFieldFromPipe();

                switch (tank) {
                    case '0':
                        mvaddch(fieldCounter / wc.getX()+1, fieldCounter % wc.getX()+1, ' ');
                        break;
                    case 'g':
                        attron(COLOR_PAIR(2));
                        mvaddch(fieldCounter / wc.getX()+1, fieldCounter % wc.getX()+1, 'X');
                        break;
                    case 'r':
                        attron(COLOR_PAIR(3));
                        mvaddch(fieldCounter / wc.getX()+1, fieldCounter % wc.getX()+1, 'X');
                        break;
                    default:
                        syslog(LOG_ERR, "illegal field input: %c", tank);
                        break;
                }
                fieldCounter++;
            }

            //check, if there is input waiting
            while (input != ERR) {
                input = getch();
                switch (input) {
                    case 'q':
                        wc.terminate(pipe);
                        exit(0);
                    case 'x':
                        wc.signalWorld(SIGINT);
                        wc.terminate(pipe);
                        break;
                    case 'r':
                        wc.signalWorld(SIGUSR1);
                    default: //in case of none or unknown input do nothing
                        break;
                }
            }
            wc.readGameBoardSize();
        }
    }
    catch (std::runtime_error &err){
        syslog(LOG_ERR, "caught exception");
    }
    return -1;
}
