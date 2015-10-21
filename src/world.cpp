#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "world.h"

using namespace std;

extern char *optarg;
extern int optind;

const char *ARGS = "h";
const struct option LONG_ARGS[] = {
	{"green-tanks", required_argument, NULL, 'g'},
	{"red-tanks", required_argument, NULL, 'r'},
	{"total-respawn", required_argument, NULL, 't'},
	{"area-size", required_argument, NULL, 'a'},
	{"help", no_argument, NULL, 'h'},
	{0, 0, 0, 0}
};

void usage()
{
	cout << "Usage:\n";
	cout << "\t--green-tanks <N>\n";
	cout << "\t\tcreates <N> green tanks\n\n";
	cout << "\t--red-tanks <N>\n";
	cout << "\t\tcreates <N> red tanks\n\n";
	cout << "\t--total-respawn <N>\n";
	cout << "\t\ta total of <N> tanks will be respawned\n\n";
	cout << "\t--area-size <N> <M>\n";
	cout << "\t\tgame area will have size <N> x <M>\n\n";
	cout << "\t-h, --help\n";
	cout << "\t\tshows this help\n\n";
}

int main(int argc, char *argv[])
{
	char opt = 0;

	//World world;
	/*world.areaX = 0;
	world.areaY = 0;
	world.greenCount = 0;*/

	while ((opt = getopt_long(argc, argv, ARGS, LONG_ARGS, NULL) ) != -1) {
		switch (opt) {
		case 'h':
			usage();
			return 0;
		case 'g':
			//world.greenCount = atol(optarg);
			break;
		case 'r':
			break;
		case 't':
			break;
		case 'a':
			/*world.areaX = atol(optarg);
			world.areaY = atol(argv[optind++]);*/
			break;
		}
	}

	//cout << world.areaX << " " << world.areaY << " " << world.greenCount << endl;

	return 0;
}
