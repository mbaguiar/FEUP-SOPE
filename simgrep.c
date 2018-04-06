#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>

#define DEFAULT_DIR    "stdin"

char * directory = "";
char * pattern = "";
bool ignoreCase = false;
bool showFileName = false;
bool lineNumbers = false;
bool lineCount = false;
bool wholeWord = false;
bool recursive = false;

void sigint_handler(int signo){
    char ans[30];
	printf("\nAre you sure you want to terminate (Y/N)? ");
    scanf("%s", ans);

	if (strcasecmp(ans, "y") == 0) exit(0);
	else if (strcasecmp(ans, "n") == 0) return;
	else sigint_handler(SIGINT);

	return;
}


int searchFile(char * path){

    char * (*compareFunc)(const char *, const char *); 
    compareFunc = (ignoreCase? &strcasestr : &strstr);

    FILE * file;
    char line[100];
    int count = 0;
    if ((file = fopen(path, "r")) == NULL){
        printf("Error opening file: %d\n", errno);
        return 1;
    }
    printf("File opened\n");

    while(fgets(line, 100, file)){
        count++;
        if (wholeWord){
            //char * res = (compareFunc)(line, pattern);
            
        } else {
            if ((compareFunc)(line, pattern)) {
                if (lineNumbers) printf("%d: ", count);
                    printf("%s", line);
                }
        }
    }
    fclose(file);
    return 0;

}

int isDirectory(char * path) {
    struct stat statbuf;
    stat(path, &statbuf);
    if(S_ISDIR(statbuf.st_mode)) {
        printf("directory\n");
        return 0;
    } else {
        printf("file\n");
        return 1;
    }

}

int loopDirectory(char * path) {
    DIR *dfd;
    struct dirent *dp;

    char filename[100];

    if ((dfd = opendir(path)) == NULL) {
        fprintf(stderr, "Cant open dir %s", path);
        return 0;
    }

    while((dp = readdir(dfd)) != NULL) {
        sprintf( filename, "%s/%s",path,dp->d_name);
        printf("filename %s\n", filename);
        if (!isDirectory(filename)) {
            continue;
        } else {
            printf("Call search file function");
        }
    }

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

    if (!isDirectory(directory)) {
        loopDirectory(directory);
    }

    //printf("pattern: %s \ndirectory: %s \nignoreCase: %d \nshowFileName: %d \nlineNumbers: %d \nlineCount: %d \nwholeWord: %d \nrecursive: %d \n", 
    //pattern, directory, ignoreCase, showFileName, lineNumbers, lineCount, wholeWord, recursive);

    searchFile(directory);

    return 0;
}



