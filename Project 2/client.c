#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <time.h>
#include "coiso.h"

unsigned int time_out;
unsigned int num_wanted_seats;
unsigned int pref_seat_list[MAX_CLI_SEATS];
unsigned int pref_seat_count = 0;
char message[1000];
int fdClog;
int fdCBook;

void createMessage() {
    sprintf(message, "%d %d ", getpid(), num_wanted_seats);
    int i;
    for (i = 0; i < pref_seat_count; i++) {
        char num[6];
        sprintf(num, "%d ", pref_seat_list[i]);
        strcat(message, num);
    }

    printf("%s\n", message);
}

void writeErrorToClog(int error) {
    char message[20];
    char *err;

    switch(error) {
        case MAX_SEAT:
            err = "MAX";
            break;
        case INVALID_NUM_WANTED_SEATS:
            err = "NST";
            break;
        case INVALID_SEAT:
            err = "IID";
            break;
        case UNAVAILABLE_SEAT:
            err = "NAV";
            break;
        case FULL:
            err = "FUL";
            break;
        case TIME_OUT:
            err = "OUT";
            break;
    }

    sprintf(message, "%d %s\n", getpid(), err);
    write(fdClog, message, strlen(message));
}

void writeSuccessToClog(int num, int seat_n, int i) {
    char message[20];
    sprintf(message, "%05d %02d.%02d %04d\n", getpid(), i, num, seat_n);
    write(fdClog, message, strlen(message));
}

void storeBookedSeat(int seat_num) {
    char message[10];
    sprintf(message, "%04d\n", seat_num);
    write(fdCBook, message, strlen(message));
}

void processAnswer(char *message) {
    int num;
    char *token;
    char s[] = " ";
    token = strtok(message, s);
    num = strtoul(token, NULL, 0);

    if (num < 0) {
        writeErrorToClog(num);
        close(fdClog);
        return;
    }

    int i;
    for(i = 1; i <= num; i++) {
        token = strtok(NULL, s);
        int seat_n = strtoul(token, NULL, 0);
        writeSuccessToClog(num, seat_n, i);
        storeBookedSeat(seat_n);
    }

    close(fdClog);
}

int main(int argc, char *argv[]){
    if (argc < 4){
        printf("Wrong no of args.\nUsage: client <time_out> <num_wanted_seats> <pref_seat_list>\n");
        return 1;
    }

    if ( (time_out = strtoul(argv[1], NULL, 0)) == 0){
        printf("Could not convert time_out.\n");
        return 1;
    }

    if ( (num_wanted_seats = strtoul(argv[2], NULL, 0)) == 0){
        printf("Could not convert num_wanted_seats.\n");
        return 1;
    }
    int i;
    for (i = 3; i < argc; i++){
        pref_seat_list[i-3] = strtoul(argv[i], NULL, 0);
        pref_seat_count++;
    }

    char fifoname[strlen(FIFO_ANS_PREFIX) + WIDTH_PID + 1];
    sprintf(fifoname, "%s%d", FIFO_ANS_PREFIX, getpid());
    
    if (mkfifo(fifoname, 0660)){
        printf("Error creating answers FIFO.\n");
        return 1;
    }

    createMessage();

    int fdrequests = open(FIFO_REQ_NAME, O_WRONLY | O_NONBLOCK);
    int len = strlen(message);
    write(fdrequests, message, len);
    close(fdrequests);

    int fdAnswer = open(fifoname, O_RDONLY | O_NONBLOCK);
    fdClog = open(CLOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0664);
    fdCBook = open(CBOOK_FILE, O_WRONLY | O_CREAT | O_APPEND, 0664);
    char str[500];

    time_t endwait;
    time_t start = time(NULL);
    time_t seconds = time_out;

    endwait = start + seconds;
    int timedOut = 1;

    do {
        int n = readline(fdAnswer, str);
        if (n) {
            printf("%s\n", str);
            processAnswer(str);
            timedOut = 0;
            break;
        }
        start = time(NULL);
    } while(start < endwait);

    if (timedOut) {
        writeErrorToClog(TIME_OUT);
    }


    close(fdCBook);
    close(fdAnswer);
    unlink(fifoname);
    return 0;
}