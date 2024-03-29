#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEFAULT_DIR    "stdin"

char * directory = "";
char * pattern = "";
bool ignoreCase = false;
bool showFileName = false;
bool lineNumbers = false;
bool lineCount = false;
bool wholeWord = false;
bool recursive = false;
int totalLineCount = 0;


void sigint_handler(int signo){
    kill(0, SIGTSTP);
    char ans[30];
    printf("\n");
    do {
        printf("Are you sure you want to terminate (Y/N)? ");
        scanf("%s", ans);
    } while(strcasecmp(ans, "y") != 0 && strcasecmp(ans, "n") != 0);
	
	if (strcasecmp(ans, "y") == 0) exit(0);
	else if (strcasecmp(ans, "n") == 0) {
        kill(0, SIGCONT);
    }
	return;
}

bool findWord(char * line){
    char * (*compareFunc)(const char *, const char *);
    compareFunc = (ignoreCase? &strcasestr : &strstr);
    if (wholeWord){
        char * word;
        if ((word = (compareFunc)(line, pattern))){
            return ((word == line || *(word - 1) == ' ' || *(word - 1) == '\0' || *(word - 1) == '\n') && 
            (*(word + strlen(pattern) ) == ' ' || *(word + strlen(pattern)) == '\0' || *(word + strlen(pattern) ) == '\n'));
        }
    } else {
        if ((compareFunc)(line, pattern)) {
            return true;
        }
    }
    return false;
}


int searchFile(char * path){
    FILE * file = NULL;
    char * line = NULL;
    int count = 0;
    int nLines = 0;
    size_t n;
    bool found = false;
    if (path != DEFAULT_DIR){
        if ((file = fopen(path, "r")) == NULL){
            printf("Error opening file: %d\n", errno);
            return 1;
        }
    } else {
        file = stdin;
    }

    while(getline(&line, &n, file) != -1){
        count++;
        found = findWord(line);
            if (found) {
                nLines++;
                totalLineCount++;
                if (!lineCount){
                    if (!showFileName) {
                        if (recursive) printf("%s:", path);
                        if (lineNumbers) printf("%d: ", count);
                        printf("%s", line);
                        if (*(line + strlen(line) - 1) != '\n') printf("\n");
                    }
                }
            }
    }

    free(line);

    if (nLines > 0 && showFileName) {
        printf("%s\n", path);
        return 0;
    }
    fclose(file);
    return 0;

}

int isDirectory(char * path) {
    struct stat statbuf;
    stat(path, &statbuf);
    if(S_ISDIR(statbuf.st_mode)) {
        return 1;
    } else {
        return 0;
    }
}

int loopDirectory(char * path) {
    DIR *dfd;
    struct dirent *dp;
    char filename[100];

    if ((dfd = opendir(path)) == NULL) {
        fprintf(stderr, "Cannot open dir %s\n", path);
        return 0;
    }

    int pid;
    while((dp = readdir(dfd)) != NULL) {
        sprintf(filename, "%s/%s",path,dp->d_name);
        if ((strcmp(dp->d_name, ".") == 0) || strcmp(dp->d_name, "..") == 0) {
            continue;
        }     
        if (isDirectory(filename)) {
             pid = fork();
            if (pid == 0) {

                struct sigaction sa;
                sa.sa_handler = SIG_DFL;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = 0;

                sigignore(SIGINT);

                if (sigaction(SIGTSTP, &sa ,NULL) == -1){
                    fprintf(stderr,"Unable to install SIGTSTP handler\n");
                    exit(1);
                }
                loopDirectory(filename);
                break;
            } else {
                continue;
            } 
        } else {
            searchFile(filename);
        } 
    }

    int status;
    waitpid(pid, &status, 0);
    return 0;

}


int main(int argc, char* argv[]){
    directory = (char *)malloc(30*sizeof(char));
    pattern = (char *)malloc(30*sizeof(char));

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa ,NULL) == -1){
        fprintf(stderr,"Unable to install SIGINT handler\n");
        exit(1);
    }

    sa.sa_handler = SIG_IGN;

    if (sigaction(SIGTSTP, &sa ,NULL) == -1){
        fprintf(stderr,"Unable to install SIGTSTP handler\n");
        exit(1);
    }

    if (argc < 3){
        pattern = argv[1];
        directory = DEFAULT_DIR;
    } else {
        int i;
        for (i = 1; i < argc; i++){

            if (strcmp(argv[i], "-i") == 0){
                ignoreCase = true;
                continue;
            }

            if (strcmp(argv[i], "-l") == 0){
                showFileName = true;
                continue;
            }

            if (strcmp(argv[i], "-n") == 0){
                lineNumbers = true;
                continue;
            }

            if (strcmp(argv[i], "-c") == 0){
                    lineCount = true;
                    continue;
            }

            if (strcmp(argv[i], "-w") == 0){
                    wholeWord = true;
                    continue;
            }
            if (strcmp(argv[i], "-r") == 0){
                    recursive = true;
                    continue;
            }

            break;
        }

        pattern = argv[i];

        if (++i < argc){
            directory = argv[i];
        } else {
            directory = DEFAULT_DIR;
        }

    }

    if (recursive && directory != DEFAULT_DIR) {
        loopDirectory(directory);
    } else {
        searchFile(directory);
    }
    if (lineCount) printf("%d\n", totalLineCount);
    return 0;
}
