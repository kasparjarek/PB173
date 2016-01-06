#include <getopt.h>
#include <libintl.h>
#include <locale.h>
#include <ncurses.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <iostream>

#define _(STRING) gettext(STRING)
using std::cout;
using std::endl;


const char *IOT_PORT = "1337";
const char *ARGS = "i:h";
const struct option LONG_ARGS[] = {
    {"ip-address", required_argument, NULL, 'i'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

int sendCmdCurrentLine = 0;
int recvCmdCurrentLine = 0;
int sockfd;

void printHelp()
{
    cout << _("Usage:") << endl;
    cout << "\t" << "-i, --ip-address <ipaddr>" << endl;
    cout << "\t\t" << _("ip address of the server (default is localhost)") << endl;

    cout << "\t" << "-h, --help" << endl;
    cout << "\t\t" << _("shows this help") << endl << endl;

    cout << "\t" << _("How to control the tank:") << endl;
    cout << "\t\t" << _("Use 'q' for exit") << endl;
    cout << "\t\t" << _("Use 'w' 'a' 's' 'd' for movement.") << endl;
    cout << "\t\t" << _("Use arrows for shooting.") << endl << endl;
}

void sendMsg(const char *const msg)
{
    attron(COLOR_PAIR(1));

    ssize_t rv = send(sockfd, msg, 2, MSG_DONTWAIT);
    if (rv == 2) {
        syslog(LOG_INFO, "send() sent msg: %s", msg);
    }
    else if (rv == -1) {
        syslog(LOG_ERR, "send() failed: %s", strerror(errno));
        attron(COLOR_PAIR(3));
    }
    else {
        syslog(LOG_ERR, "send() sent %d chars instead of %d", (int) rv, 2);
    }
}

void printSendCmd(const char *const msg)
{
    if (sendCmdCurrentLine == LINES) {
        clear();
        sendCmdCurrentLine = 0;
        recvCmdCurrentLine = 0;
    }
    mvprintw(sendCmdCurrentLine++, 0, _("Sending command: %s"), msg);
}

void printRecvCmd(char *msg) {
    attron(COLOR_PAIR(2));
    if (recvCmdCurrentLine == LINES) {
        clear();
        sendCmdCurrentLine = 0;
        recvCmdCurrentLine = 0;
    }
    mvprintw(recvCmdCurrentLine++, COLS / 2, _("World received: %s"), msg);
}

int readInput()
{
    int input = getch();
    switch (input){

        // Quit
        case 'q':
            return -1;

        // Send move actions
        case 'w':
            sendMsg("mu");
            printSendCmd(_("Move Up"));
            break;
        case 's':
            sendMsg("md");
            printSendCmd(_("Move Down"));
            break;
        case 'a':
            sendMsg("ml");
            printSendCmd(_("Move Left"));
            break;
        case 'd':
            sendMsg("mr");
            printSendCmd(_("Move Right"));
            break;

        // Send fire actions
        case KEY_LEFT:
            sendMsg("fl");
            printSendCmd(_("Fire Left"));
            break;
        case KEY_RIGHT:
            sendMsg("fr");
            printSendCmd(_("Fire Right"));
            break;
        case KEY_UP:
            sendMsg("fu");
            printSendCmd(_("Fire Up"));
            break;
        case KEY_DOWN:
            sendMsg("fd");
            printSendCmd(_("Fire Down"));
            break;

        // In case of none or unknown input do nothing
        default:
            break;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    bindtextdomain("tankclient", "../locale");
    textdomain("tankclient");

    int opt = 0;
    std::string ip_address = "127.0.0.1";

    while ((opt = getopt_long(argc, argv, ARGS, LONG_ARGS, NULL)) != -1) {
        switch (opt) {
        case 'i':
            ip_address = optarg;
            break;
        case 'h':
            printHelp();
            return 0;
        default:
            printHelp();
            exit(1);
        }
    }

    // Prepare server info

    struct addrinfo hints;
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int rv;
    if ((rv = getaddrinfo(ip_address.c_str(), IOT_PORT, &hints, &servinfo)) != 0) {
        syslog(LOG_ERR, "getaddrinfo() failed: %s", gai_strerror(rv));
        exit(1);
    }

    // Create socket

    struct addrinfo *p;
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) != -1) {
            break;
        }
    }

    if (p == NULL) {
        syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
        exit(1);
    }

    // Make Connection

    if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        syslog(LOG_ERR, "connect() failed: %s", strerror(errno));
    }

    // Start ncurses

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(500);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    // Listen for input and receive msg from socket

    char buff[3];
    buff[2] = 0;
    while (readInput() == 0) {

        ssize_t recvRet = recv(sockfd, buff, 2, MSG_DONTWAIT);

        if (recvRet == -1) {
            syslog(LOG_ERR, "recv() failed: %s", strerror(errno));
        }
        else if (recvRet != 2) {
            syslog(LOG_ERR, "recv() read %d chars instead of %d", (int) recvRet, 2);
        }
        else {
            printRecvCmd(buff);
        }
    }

    // Clean up

    endwin();
    close(sockfd);
    freeaddrinfo(servinfo);
    exit(0);
}
