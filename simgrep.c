#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define DEFAULT_DIR     "stdin"

char * directory = "";
char * pattern = "";
bool ignoreCase = false;
bool showFileName = false;
bool lineNumbers = false;
bool lineCount = false;
bool wholeWord = false;
bool recursive = false;

int searchFile(char * path){
    FILE * file;
    char * line = NULL;
    size_t len = 0;
    if ((file = fopen(path, "r")) == NULL){
        return 1;
    }

    //while ()

    

}


int main(int argc, char* argv[]){



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

    printf("pattern: %s \ndirectory: %s \nmatchCase: %d \nshowFileName: %d \nlineNumbers: %d \nlineCount: %d \nwholeWord: %d \nrecursive: %d \n ", 
    pattern, directory, ignoreCase, showFileName, lineNumbers, lineCount, wholeWord, recursive);

    return 0;
}



