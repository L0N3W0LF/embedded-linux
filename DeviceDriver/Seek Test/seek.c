#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define DEV0_NAME        "/dev/device0"

int main (int argc, char* argv[])
{
    int fd;
    FILE *fp = fopen(DEV0_NAME,"w+");
    fd = fileno(fp);

    lseek(fd, 2, SEEK_CUR);

    return 0;
}