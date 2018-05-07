#include <unistd.h>

#define MAX_CLI_SEATS 99
#define DELAY()
#define WIDTH_PID 5
#define WIDTH_XXNN 5
#define WIDTH_SEAT 4
#define MAX_ROOM_SEATS 9999
#define FIFO_REQ_NAME "requests"
#define FIFO_ANS_PREFIX "ans"

int readline(int fd, char *str){
    int n;
    int num = 0;
    do {
        n = read(fd,str,1);
        if (n) num++;
    } while (n>0 && *str++ != '\0');
    return (num);
}