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
#define CBOOK_FILE "cbook.txt"
#define MAX_SEAT -1
#define INVALID_NUM_WANTED_SEATS -2
#define INVALID_SEAT -3
#define INVALID_PARAM -4
#define UNAVAILABLE_SEAT -5
#define FULL -6
#define TIME_OUT -7
#define SERVER_CLOSED "SERVER CLOSE\n"
#define macroToString(x) toString(x)
#define toString(x) #x
#define FILLER(w) "%0" macroToString(w) "d"
#define SLOG_SEAT_FORMAT " " FILLER(WIDTH_SEAT)
#define LOG_BOOKED_SEATS_FORMAT FILLER(WIDTH_SEAT) "\n"
#define SLOG_BOOK_FORMAT FILLER(2) "-" FILLER(WIDTH_PID) "-" FILLER(2)
#define CLOG_SUCCESS_BOOK_FORMAT FILLER(WIDTH_PID) " " FILLER(2) "." FILLER(2) " " FILLER(WIDTH_SEAT) "\n"
#define CLOG_FAIL_BOOK_FORMAT FILLER(WIDTH_PID) " %s\n"
#define SLOG_OFFICE_OPEN FILLER(2) "-OPEN\n"
#define SLOG_OFFICE_CLOSE FILLER(2) "-CLOSE\n"


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