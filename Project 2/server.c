#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h> 
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "helper.h"

typedef struct {
    int clientId;
    int num;
} Seat;

unsigned int num_room_seats;
unsigned int num_ticket_offices;
unsigned int open_time;
Seat seats[MAX_ROOM_SEATS];
Request *buffer;
sem_t *buffer_empty, *buffer_full;
pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t book_lock = PTHREAD_MUTEX_INITIALIZER;
int fdSlog;
int fdrequests;
pthread_t *threads;
int closeTicketOffices = 0;


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
        request.num_wanted_seats++;
    }
    *buffer = request;
}

void storeRequest(char *message) {
    processString(message);
}

int isSeatFree(Seat *seats, int seatNum) {
    DELAY();
    return (seats[seatNum - 1].clientId == 0);
}

void bookSeat(Seat *seats, int seatNum, int clientId) {
    seats[seatNum - 1].clientId = clientId;
    DELAY();
}

void freeSeat(Seat *seats, int seatNum) {
    seats[seatNum - 1].clientId = 0;
    DELAY();
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
        return INVALID_NUM_WANTED_SEATS;

    int i;
    for (i = 0; i < request.num_wanted_seats; i++) {
        if (request.seats[i] <= 0 || request.seats[i] > num_room_seats)
            return INVALID_SEAT;
    }

    return 0;
}

int isRoomFull() {
    int i;
    for (i = 0; i < num_room_seats; i++) {
        if (seats[i].clientId == 0)
        return 0;
    }
    return 1;
}

void sendAnswerToClient(char* answer, int idClient) {
    char fifoname[strlen(FIFO_ANS_PREFIX) + WIDTH_PID + 1];
    sprintf(fifoname, "%s%d", FIFO_ANS_PREFIX, idClient);
    int fdAnswer = open(fifoname, O_WRONLY | O_NONBLOCK);
    write(fdAnswer, answer, strlen(answer));
    close(fdAnswer);
} 

void writeErrorToSlog(Request request, int error, int t) {
    char message[1000];
    sprintf(message, SLOG_BOOK_FORMAT, t, request.clientId, request.num_seats);
    int i;
    for (i = 0; i < request.num_wanted_seats; i++) {
       char temp[6];
       sprintf(temp, SLOG_SEAT_FORMAT, request.seats[i]);
       strcat(message, temp);
    }
    switch(error) {
        case MAX_SEAT:
            strcat(message, " - MAX\n");
            break;
        case INVALID_NUM_WANTED_SEATS:
            strcat(message, " - NST\n");
            break;
        case INVALID_SEAT:
            strcat(message, " - IID\n");
            break;
        case INVALID_PARAM:
            strcat(message, " - ERR\n");
            break;
        case UNAVAILABLE_SEAT:
            strcat(message, " - NAV\n");
            break;
        case FULL:
            strcat(message, " - FUL\n");
            break;

    }

    write(fdSlog, message, strlen(message));
}

void sendErrorAnswerToClient(int error, int clientId) {
   char message[3];
   sprintf(message, "%d", error);
   sendAnswerToClient(message, clientId);
}

void sendSuccessAnswerToClient(int *booked_seats, int num_seats, int clientId) {
    char message[1000];
    sprintf(message, "%d", num_seats);
    int i;
    for (i = 0; i < num_seats; i++) {
        char temp[6];
        sprintf(temp, " %d", booked_seats[i]);
        strcat(message, temp);
    }

    strcat(message, "\n");
    sendAnswerToClient(message, clientId);
}

void writeSuccessToSlog(Request request, int *booked_seats, int t) {

    char message[1000];

    sprintf(message, SLOG_BOOK_FORMAT, t, request.clientId, request.num_seats);

    int i;
    for (i = 0; i < request.num_wanted_seats; i++) {
       char temp[6];
       sprintf(temp, SLOG_SEAT_FORMAT, request.seats[i]);
       strcat(message, temp);
    }
    strcat(message, " -");

    for (i = 0; i < request.num_seats; i++) {
        char temp[6];
        sprintf(temp, SLOG_SEAT_FORMAT, booked_seats[i]);
        strcat(message, temp);
    }
    strcat(message, "\n");
    write(fdSlog, message, strlen(message));
}

void bookRequest(Request request, int thread_num) {
    int booked_seats_num = 0;
    int booked_seats[request.num_seats];
    int i;
    for (i = 0; i < request.num_wanted_seats; i++) {

        if(isSeatFree(seats, request.seats[i])) {
            bookSeat(seats, request.seats[i], request.clientId);
            booked_seats[booked_seats_num] = request.seats[i];
            booked_seats_num++;
            
        }

        if (booked_seats_num == request.num_seats) {
            writeSuccessToSlog(request, booked_seats, thread_num);
            sendSuccessAnswerToClient(booked_seats, booked_seats_num, request.clientId);
            return;
        }
    }
    
    for (i = 0; i < booked_seats_num; i++) {
        freeSeat(seats, booked_seats[i]); 
    }
    writeErrorToSlog(request, UNAVAILABLE_SEAT, thread_num);
    sendErrorAnswerToClient(UNAVAILABLE_SEAT, request.clientId);
    
}


void *waitForRequest(void *threadnum) {
    
    char message[10];
    Request request;
    
    sprintf(message, SLOG_OFFICE_OPEN, *(int *) threadnum);
    write(fdSlog, message, strlen(message));

    while(!closeTicketOffices) {
        pthread_mutex_lock(&buffer_lock);
         if (closeTicketOffices) {
            pthread_mutex_unlock(&buffer_lock);
            continue;
        } 

         if (sem_trywait(buffer_full) == -1){
            if (errno == EAGAIN){
                pthread_mutex_unlock(&buffer_lock);
                continue;
            }    
        }

        request = *buffer;
        sem_post(buffer_empty);
        pthread_mutex_unlock(&buffer_lock);  

        int result = validateRequest(request);
        if (result != 0) {
            writeErrorToSlog(request, result, *(int *) threadnum);
            sendErrorAnswerToClient(result, request.clientId);
            continue;
        }
        else if (isRoomFull()) {
            writeErrorToSlog(request, FULL, *(int *) threadnum);
            sendErrorAnswerToClient(result, request.clientId);
            continue;
        }

        pthread_mutex_lock(&book_lock);
        bookRequest(request, *(int *) threadnum);
        pthread_mutex_unlock(&book_lock);
    }

    sprintf(message, SLOG_OFFICE_CLOSE, *(int *) threadnum);
    write(fdSlog, message, strlen(message));
    pthread_exit(NULL);
}

void storeBookedSeats() {
    int i;
    int fdSBook = open(SBOOK_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    for (i = 0; i < num_room_seats; i++) {
        char temp[10];
        if (seats[i].clientId) {
            sprintf(temp, LOG_BOOKED_SEATS_FORMAT, seats[i].num);
            write(fdSBook, temp, strlen(temp));
        }
    }
    close(fdSBook);
}

void shutdown(){
    int t;
    for (t = 0; t < num_ticket_offices; t++) {
        pthread_join(threads[t], NULL);
    }
    storeBookedSeats();
    write(fdSlog, SERVER_CLOSED, strlen(SERVER_CLOSED));
    close(fdSlog);
    close(fdrequests);
    unlink(FIFO_REQ_NAME);
    pthread_mutex_destroy(&buffer_lock);
    pthread_mutex_destroy(&book_lock);
    sem_close(buffer_empty);
    sem_unlink(BUF_EMPTY);
    sem_close(buffer_full);
    sem_unlink(BUF_FULL);
    exit(0);
}

void timeout_handler(int signo) {
    closeTicketOffices = 1;    

    shutdown();
}


int main(int argc, char *argv[]){
    if (argc != 4) {
        printf("Wrong no of args.\nUsage: server <num_room_seats> <num_ticket_offices> <open_time>\n");
        return 1;
    }

    if ( (num_room_seats = strtoul(argv[1], NULL, 0)) == 0){
        printf("Could not convert num_room_seats.\n");
        return 1;
    } else if (num_room_seats > MAX_ROOM_SEATS){
        printf("num_room_seats greater than MAX_ROOM_SEATS.\n");
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

    struct sigaction sa;
    sa.sa_handler = timeout_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        fprintf(stderr,"Unable to install SIGALRM handler\n");
        exit(1);
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr,"Unable to install SIGINT handler\n");
        exit(1);
    }

    setupSeats();
    buffer = (Request *) malloc(sizeof(Request));

    pthread_t temp[num_ticket_offices];
    threads = temp;
    int threads_num[num_ticket_offices];
    int t;

    fdSlog = open(SLOG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0664);

    int fdClog = open(CLOG_FILE, O_WRONLY | O_TRUNC);
    close(fdClog);
    
    int fdCbook = open(CBOOK_FILE, O_WRONLY | O_TRUNC);
    close(fdCbook);

    buffer_empty = sem_open(BUF_EMPTY, O_CREAT, 0600, 1);
    buffer_full = sem_open(BUF_FULL, O_CREAT, 0600, 0);

    for (t = 1; t <= num_ticket_offices; t++) {
        threads_num[t-1] = t;
        pthread_create(&threads[t-1], NULL, waitForRequest, (void *) &threads_num[t-1]);
    }


    alarm(open_time);

    fdrequests = open(FIFO_REQ_NAME, O_RDONLY);

    char str[500];

    do {
        int n = readline(fdrequests, str);
        if (n) {
            sem_wait(buffer_empty);
            storeRequest(str);
            sem_post(buffer_full);
        }
    }
    while(1);
    return 0;
}