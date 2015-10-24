#include <iostream>
#include <stdio.h>
#include <syslog.h>


int main(void)
{
    syslog(LOG_INFO, "Tank with pid %d was created\n", getpid());
    sleep(2);
    return 0;
}
