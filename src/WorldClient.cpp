#include "WorldClient.h"

using namespace std;

const struct option LONG_ARGS[] = {
        {"usage", no_argument, NULL, 'h'},
        {"pipe", required_argument, NULL, 'p'},
        {0, 0, 0, 0}
};

void usage(){
    cout << "Usage:" << endl;
    cout << "\t" << "--pipe <path>" << endl;
    cout << "\t" << "path to named pipe of world program" << endl;
    cout << "\t" << "--usage" << endl;
}

WorldClient::WorldClient(char *path): y(0),
                                      x(0),
                                      destroyed_green_tanks(0),
                                      destroyed_red_tanks(0),
                                      green_tanks(0),
                                      red_tanks(0) {
    //open pipe to world
    int pipe_input = open(path, O_NONBLOCK | O_RDONLY);
    if(pipe_input == -1){
        syslog(LOG_ERR, "couldn't open pipe to world");
        throw runtime_error("couldn't open pipe to world");
    }
    char buffer[512];
    read(pipe_input, buffer, 512);
    sscanf(buffer, "%d, %d", &x, &y);
}

int WorldClient::initGameboard() {
    //start ncurses
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1));
    refresh();

    //print game frame
    for(int curY = 0; curY <= y+1; curY++){
        for(int curX  = 0; curX <= x+1; curX++){
            if(curX == 0 || curX == x+1 || curY == 0 || curY == y+1){
                mvaddch(curY, curX, '$');
                move(0,0);
            }
        }
    }

    //print statistics
    mvaddstr(y+3, 0, "Destroyed tanks: ");
    attron(COLOR_PAIR(2));
    mvaddch(y+4, 0, '0');

    attron(COLOR_PAIR(3));
    mvaddch(y+5, 0, '0');
    refresh();
    return 0;
}

int WorldClient::terminate() {
    endwin();
    exit(0);
}

int WorldClient::getBoardDataStart(char * buffer){
    char * pos = strtok(buffer, ",");
    pos = strtok(NULL, ",");
    pos = strtok(NULL, ",");
    if(pos == nullptr){
        syslog(LOG_ERR, "couldn't read coord data from pipe");
        return -1;
    }
    coord_offset = (int) (pos-buffer);
    return 0;
}



//send sigint to world process
int WorldClient::terminateWorld(int signal) {
    pid_t pid = 0;
    ifstream ifs (WORLD_PATH);
    ifs >> pid;
    if(pid){
        kill(pid, signal);
        return 0;
    }
    syslog(LOG_ERR, "couldn't read world pid");
    return -1;
}

int main(int argc, char ** argv){

    char * pipe = NULL;
    char opt;
    while ((opt = getopt_long(argc, argv, "ph", LONG_ARGS, NULL)) != -1) {
        switch (opt) {
            case 'p': //pipe
                pipe = optarg;
                break;
            case 'h': //pipe
                usage();
                break;
            default:
                usage();
                exit(1);
        }
    }

    WorldClient wc (pipe);
    wc.initGameboard();

    char input;
    timeout(0);
    while((input = getch()) != 'q'){
        switch (input){
            case 'x':
                wc.terminateWorld(SIGINT);
                break;
            case 'r':
                wc.terminateWorld(SIGUSR1);
                break;
            default:
                break;
        }
        char buffer[wc.getX()*wc.getY()*2];
        if((pread(wc.getPipe_input(), buffer, (size_t) (wc.getX()*wc.getY()*2), (int) wc.getCoord_offset())) == -1){
            syslog(LOG_ERR, "couldn't read pipe data");
            return -1;
        }
        for(int i = 0; i < wc.getY()*wc.getX(); i++){
            char tank;
            sscanf(buffer, ",%c", &tank);
            switch (tank) {
                case '0':
                    mvaddch(i / wc.getX(), i % wc.getX(), ' ');
                    break;
                case 'g':
                    attron(COLOR_PAIR(2));
                    mvaddch(i / wc.getX(), i % wc.getX(), 'X');
                    break;
                case 'r':
                    attron(COLOR_PAIR(3));
                    mvaddch(i / wc.getX(), i % wc.getX(), 'X');
                    break;
                default:
                    break;
            }
        }
    }
    wc.terminate();
}
