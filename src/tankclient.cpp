#include <iostream>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <cstring>

using namespace std;

const char *IOT_PORT = "1337";
const char *ARGS = "i:h";
const struct option LONG_ARGS[] = {
    {"ip-address", required_argument, NULL, 'i'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

void printHelp()
{
    cout << "\t" << "-i, --ip-address <ipaddr>" << endl;
    cout << "\t\t" << "ip address of the server (default is localhost" << endl;

    cout << "\t" << "-h, --help" << endl;
    cout << "\t\t" << "shows this help" << endl << endl;
}

int main(int argc, char *argv[])
{
    char opt = 0;
    string ip_address = "127.0.0.1";

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

    int rv;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(ip_address.c_str(), IOT_PORT, &hints, &servinfo)) != 0) {
        syslog(LOG_ERR, "getaddrinfo() failed: %s", gai_strerror(rv));
        exit(1);
    }

    int sockfd;
    struct addrinfo *p;
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        break;
    }

    if (p == NULL) {
        syslog(LOG_ERR, "socket() failed: %s", strerror(errno));
        exit(1);
    }

    /* sendto sockfd */

    freeaddrinfo(servinfo);
    exit(0);
}
