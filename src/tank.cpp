#include <ctime>
#include <iostream>
#include <stdexcept>
#include <getopt.h>
#include <syslog.h>
#include <unistd.h>

using namespace std;

extern char *optarg;

const char *ARGS = "h";
const struct option LONG_ARGS[] = {
    {"sleep-max", required_argument, NULL, 's'},
    {"sleep-min", required_argument, NULL, 't'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

void usage()
{
    cout << "Usage:" << endl;
    cout << "\t" << "--sleep-max <N>" << endl;
    cout << "\t\t" << "maximum sleep time in seconds" << endl << endl;
    cout << "\t" << "--sleep-min <N>" << endl;
    cout << "\t\t" << "minimum sleep time in seconds" << endl << endl;
    cout << "\t" << "-h, --help" << endl;
    cout << "\t\t" << "shows this help" << endl << endl;
}

int main(int argc, char *argv[])
{
    char opt = 0;
    int sleepMin = -1;
    int sleepMax = -1;

    while ((opt = getopt_long(argc, argv, ARGS, LONG_ARGS, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        case 's':
            sleepMax = atoi(optarg);
            break;
        case 't':
            sleepMin = atoi(optarg);
            break;
        default:
            usage();
            abort();
        }
    }

    if (sleepMax < 0 || sleepMin < 0 || sleepMax < sleepMin) {
        throw runtime_error("invalid parameters");
    }

    srand(getpid());

    /* Random sleep time in microseconds with precision to miliseconds */
    size_t usec = ((rand() % ((sleepMax - sleepMin) * 1000)) + sleepMin * 1000) * 1000;

    syslog(LOG_INFO, "Tank with pid %d was created\n", getpid());
//    cout << "tank " << getpid() << " created" << endl;

    usleep(usec);
    /* Just some demo, showing problem with rand() */
//    cout << "tank " << getpid() << " destroyed, lasted " << usec / 1000000.0 << " sec" << endl;

    return 0;
}
