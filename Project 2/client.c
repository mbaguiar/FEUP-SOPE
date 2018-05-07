#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include "coiso.h"

unsigned int time_out;
unsigned int num_wanted_seats;
unsigned int pref_seat_list[MAX_CLI_SEATS];
unsigned int pref_seat_count = 0;


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

    int fdrequests = open(FIFO_REQ_NAME, O_WRONLY);
    int len = strlen("loladamixeroni");
    write(fdrequests, "loladamixeroni", len);
    close(fdrequests);
    unlink(fifoname);
    return 0;
}