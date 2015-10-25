#include <iostream>
#include <getopt.h>

using namespace std;

const char *ARGS = "h";
const struct option LONG_ARGS[] = {
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0}
};

void usage()
{
    cout << "\t-h, --help\n";
    cout << "\t\tshows this help\n\n";
}

int main(int argc, char *argv[])
{
    char opt = 0;

    while ((opt = getopt_long(argc, argv, ARGS, LONG_ARGS, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            usage();
            abort();
        }
    }

    return 0;
}
