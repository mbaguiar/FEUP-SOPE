#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <string.h>
#include "coiso.h"

unsigned int num_room_seats;
unsigned int num_ticket_offices;
unsigned int open_time;

int main(int argc, char *argv[]){
    if (argc != 4) {
        printf("Wrong no of args.\nUsage: server <num_room_seats> <num_ticket_offices> <open_time>\n");
        return 1;
    }

    if ( (num_room_seats = strtoul(argv[1], NULL, 0)) == 0){
        printf("Could not convert num_room_seats.\n");
        return 1;
    }

    if ( (num_ticket_offices = strtoul(argv[2], NULL, 0)) == 0){
        printf("Could not convert num_ticket_offices.\n");
        return 1;
    }

    if ( (open_time = strtoul(argv[3], NULL, 0)) == 0){
        printf("Could not convert open_time.\n");
        return 1;
    }

    if (mkfifo(FIFO_REQ_NAME, 0660)){
        printf("Error creating requests FIFO.\n");
        return 1;
    }

    int fdrequests = open(FIFO_REQ_NAME, O_RDONLY);

    char str[100];

    printf("antes do while\n");

    do {
        int n = readline(fdrequests, str);
        if (n) printf("%s\n", str);
    }
    while(1);

    printf("fora do while\n");
    
    close(fdrequests);
    unlink(FIFO_REQ_NAME);
    //printf("nrs: %d\nnto: %d\not: %d\n", num_room_seats, num_ticket_offices, open_time);

    return 0;

}