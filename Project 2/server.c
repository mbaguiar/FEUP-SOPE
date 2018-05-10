#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "coiso.h"

typedef struct {
    int clientId;
} Seat;

unsigned int num_room_seats;
unsigned int num_ticket_offices;
unsigned int open_time;
Seat seats[MAX_CLI_SEATS];
//mudar eventualmente nao sei o que é suposto fazer
Request *buffer;
int bufferCount = 0;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t slots_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t slots_cond = PTHREAD_COND_INITIALIZER;


void setupSeats() {
    int i;
    Seat seat;
    seat.clientId = 0;
    for (i = 0; i < num_room_seats; i++) {
        seats[i] = seat;
    }
}

void processString(char *message) {

    Request request;
    const char s[2] = " ";
    char *token;
    token = strtok(message, s);
    request.clientId = strtoul(token, NULL, 0);
    token = strtok(NULL, s);
    request.num_seats = strtoul(token, NULL, 0);
    
    request.num_wanted_seats = 0;

    while(token != NULL) {
        token = strtok(NULL, s);
        if (token == NULL) break;
        request.seats[request.num_wanted_seats] = strtoul(token, NULL, 0);
        //printf("%d ", request.seats[request.num_wanted_seats]);
        request.num_wanted_seats++;
        //printf("%d\n", request.num_wanted_seats);
    }

    *buffer = request;
    //printf("%d\n", buffer->clientId);
}

void storeRequest(char *message) {
    processString(message);
}

int isSeatFree(Seat *seats, int seatNum) {
    DELAY();
    return 0;
}

void *waitForRequest(void *threadnum) {
    //sleep(1);
    printf("Hello from thread no. %d!\n", *(int *) threadnum);

    while(1) {

        pthread_mutex_lock(&buffer_lock);
        while(bufferCount != 1)
            pthread_cond_wait(&slots_cond, &buffer_lock);
        
        bufferCount = 0;
        printf("%d\n", buffer->clientId);

        pthread_mutex_unlock(&buffer_lock);  
    }

    pthread_exit(NULL);
}

void bookSeat(Seat *seats, int seatNum, int clientId) {
    DELAY();
}

void freeSeat(Seat *seats, int seatNum) {
    DELAY();
}


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

    setupSeats();
    buffer = (Request *)malloc (sizeof(Request));

    pthread_t threads[num_ticket_offices];
    int t;
    for (t = 0; t < num_ticket_offices; t++) {
        pthread_create(&threads[t], NULL, waitForRequest, (void *) &t);
        //pthread_join(threads[t], NULL);
    }

    int fdrequests = open(FIFO_REQ_NAME, O_RDONLY);

    char str[500];

    printf("antes do while\n");

    do {
        int n = readline(fdrequests, str);
        if (n) {
            printf("%s\n", str);
            if (bufferCount != 1) {
                storeRequest(str);
                bufferCount = 1;
                pthread_cond_signal(&slots_cond);
            }
        }
    }
    while(1);

    printf("fora do while\n");
    
    close(fdrequests);
    unlink(FIFO_REQ_NAME);
    //printf("nrs: %d\nnto: %d\not: %d\n", num_room_seats, num_ticket_offices, open_time);

    return 0;

}