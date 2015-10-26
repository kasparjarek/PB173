#ifndef INTERNET_OF_TANKS_TANKBEAN_H
#define INTERNET_OF_TANKS_TANKBEAN_H

#include <utility>

enum Team
{
    GREEN, RED
};

typedef std::pair<int, int> Coordinates;

struct TankBean
{
    Coordinates position;
    Team team;
    pid_t pid;
};

#endif //INTERNET_OF_TANKS_TANKBEAN_H
