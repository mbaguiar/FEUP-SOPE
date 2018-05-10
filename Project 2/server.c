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
    int num;
} Seat;

unsigned int num_room_seats;
unsigned int num_ticket_offices;
unsigned int open_time;
Seat seats[MAX_CLI_SEATS];
//mudar eventualmente nao sei o que é suposto fazer
Request *buffer;
int bufferCount = 0;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t num_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t slots_cond = PTHREAD_COND_INITIALIZER;
int fdSlog;


void setupSeats() {
    int i;
    Seat seat;
    seat.clientId = 0;
    for (i = 0; i < num_room_seats; i++) {
        seat.num = i+1;
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
    return (seats[seatNum - 1].clientId = 0);
}

int validateRequest(Request request) {
    if (request.num_seats > MAX_CLI_SEATS)
        return MAX_SEAT;
    if (request.num_wanted_seats > MAX_CLI_SEATS || request.num_wanted_seats <= 0)
        return INVALID_NUM_WANTED_SEATS;
    if (request.num_seats <= 0)
        return INVALID_PARAM;
    if (request.num_seats > num_room_seats)
        return INVALID_PARAM;
    if (request.num_wanted_seats < request.num_seats)
        return INVALID_PARAM;

    int i;
    for (i = 0; i < request.num_wanted_seats; i++) {
        if (request.seats[i] <= 0 || request.seats[i] > num_room_seats)
            return INVALID_SEAT;
    }

    return 0;
}

void writeErrorToSlog(Request request, int error, int t) {
    char message[1000];

    sprintf(message, "%02d-%05d-%02d:", t, request.clientId, request.num_seats);

    int i;

    for (i = 0; i < request.num_wanted_seats; i++) {
       char temp[6];
       sprintf(temp, " %04d", request.seats[i]);
       strcat(message, temp);
    }
    switch(error) {
        case -1:
            strcat(message, " - MAX\n");
            break;
        case -2:
            strcat(message, " - NST\n");
            break;
        case -3:
            strcat(message, " - IID\n");
            break;
        case -4:
            strcat(message, " - ERR\n");
            break;
        case -5:
            strcat(message, " - NAV\n");
            break;
        case -6:
            strcat(message, " - FUL\n");

    }

    write(fdSlog, message, strlen(message));
}

void *waitForRequest(void *threadnum) {
    
    char message[10];
    Request request;
    
    sprintf(message, "%02d-OPEN\n", *(int *) threadnum);
    

    write(fdSlog, message, strlen(message));

    while(1) {

        pthread_mutex_lock(&buffer_lock);
        while(bufferCount != 1)
            pthread_cond_wait(&slots_cond, &buffer_lock);
        
        bufferCount = 0;
        request = *buffer;
        printf("%d\n", buffer->clientId);

        pthread_mutex_unlock(&buffer_lock);  

        int result = validateRequest(request);
        if (result != 0)
            writeErrorToSlog(request, result, *(int *) threadnum);

    }

    sprintf(message, "%02d-CLOSE\n", *(int *) threadnum);

    write(fdSlog, message, strlen(message));
    pthread_exit(NULL);
}

void bookSeat(Seat *seats, int seatNum, int clientId) {
    seats[seatNum - 1].clientId = clientId;
    DELAY();
}

void freeSeat(Seat *seats, int seatNum) {
    seats[seatNum - 1].clientId = 0;
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
    int threads_num[num_ticket_offices];
    int t = 1;

    fdSlog = open(SLOG_FILE, O_WRONLY | O_TRUNC | O_CREAT, 0664);


    for (t = 1; t <= num_ticket_offices; t++) {
        threads_num[t-1] = t;
        pthread_create(&threads[t-1], NULL, waitForRequest, (void *) &threads_num[t-1]);
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
    pthread_mutex_destroy(&buffer_lock);
    pthread_cond_destroy(&slots_cond);
    //printf("nrs: %d\nnto: %d\not: %d\n", num_room_seats, num_ticket_offices, open_time);

    return 0;

}