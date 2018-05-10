#include <unistd.h>

#define MAX_CLI_SEATS 99
#define DELAY()
#define WIDTH_PID 5
#define WIDTH_XXNN 5
#define WIDTH_SEAT 4
#define MAX_ROOM_SEATS 9999
#define FIFO_REQ_NAME "requests"
#define FIFO_ANS_PREFIX "ans"
#define SLOG_FILE "slog.txt"
#define SBOOK_FILE "sbook.txt"
#define CLOG_FILE "clog.txt"
#define MAX_SEAT -1
#define INVALID_NUM_WANTED_SEATS -2
#define INVALID_SEAT -3
#define INVALID_PARAM -4
#define UNAVAILABLE_SEAT -5
#define FULL -6

typedef struct {
    int clientId;
    int num_seats;
    int num_wanted_seats;
    int seats[MAX_CLI_SEATS];
} Request;

int readline(int fd, char *str){
    int n;
    int num = 0;
    do {
        n = read(fd,str,1);
        if (n) num++;
    } while (n>0 && *str++ != '\0');
    str[num-1] = '\0';
    return (num);
}