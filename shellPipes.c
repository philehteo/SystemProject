#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

//some color for display
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_BOLD   "\x1b[1m"
#define ANSI_FAINT   "\x1b[2m"
#define ANSI_ITALIC   "\x1b[3m"

#define MAX_COMMAND_LENGTH 100
#define MAX_PIPE 10

void printArguments(char* args[]) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("Argument %d: %s\n", i, args[i]);
    }
}

void executePipe(char* args[MAX_PIPE][MAX_COMMAND_LENGTH], int num_commands){
    num_commands += 1;
    int pipes[num_commands][2];//array of all file director that will be used

    for (size_t i = 0; i < num_commands; i++)
    {
        if(i < num_commands - 1){
                pipe(pipes[i]);
            }
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process

            if (i > 0) {//if not the first command read the output of previous process
                if( (dup2(pipes[i-1][0], STDIN_FILENO)) <0){
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }

            if (i < num_commands - 1) {//if not the last cmd writ output in your pipe array for next process
                close(pipes[i][0]);
                if(dup2(pipes[i][1], STDOUT_FILENO)<0){
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(pipes[i][1]);
            }

            execvp(args[i][0],args[i]);//exectute cmd with adequat IO
            perror("execvp");
            exit(EXIT_FAILURE);

        }
        else{

            if (i > 0) {//close the actual fd before going to next
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }

            wait(NULL);//wait for the children to be exectued before starting the next children
        }
    }

    if(num_commands > 1){
        close(pipes[num_commands-1][0]);
        close(pipes[num_commands-1][1]);
    } //close the last IO fd


}


void changeDirectory(char* path) {
    if (chdir(path) == 0) {
        getcwd(path,MAX_COMMAND_LENGTH);
        printf("Directory changed to: %s\n", path);
    } else {
        fprintf(stderr, "Failed to change directory to: %s\n", path);
    }
}

void ChangeOutput(char* filename){
    int fileDescriptor = open(filename,  O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    dup2(fileDescriptor, STDOUT_FILENO);
    close(fileDescriptor);
}

void ChangeInput(char* filename){
    int fileDescriptor = open(filename,  O_RDONLY);
    dup2(fileDescriptor, STDIN_FILENO);
    close(fileDescriptor);
}


int main() {
    char command[MAX_COMMAND_LENGTH];
    char * args[MAX_PIPE][MAX_COMMAND_LENGTH];  // +1 for NULL terminator

    int originalStdout = dup(STDOUT_FILENO);

    printf("\e[1;1H\e[2J");
    printf("---------Welcome to "ANSI_BOLD "Cyril"ANSI_COLOR_RESET" and "ANSI_BOLD"Theophile"ANSI_COLOR_RESET"'s terminal------------\n");
    printf("Here are some command you can execute : \n");
    printf(ANSI_BOLD ANSI_COLOR_MAGENTA  "ls "ANSI_COLOR_RESET"to show children directories \n");
    printf(ANSI_BOLD ANSI_COLOR_MAGENTA  "cd "ANSI_COLOR_RESET" follow by the directory you want to go in, to change directory \n");
    printf(ANSI_BOLD ANSI_COLOR_MAGENTA "cmd > or < file.txt " ANSI_COLOR_RESET "to redirect IO of the process \n");
    printf(ANSI_BOLD ANSI_COLOR_MAGENTA "cmd1 | cmd2 | cmd3 " ANSI_COLOR_RESET "to pipe up to 10 commands\n");
    printf(ANSI_BOLD ANSI_COLOR_MAGENTA "./executableFile " ANSI_COLOR_RESET "to execute a file "ANSI_ITALIC ANSI_FAINT"(you have to be in the correct working directory)\n"ANSI_COLOR_RESET);
    printf(" \n");
    printf(" \n");
    char* lastWord = "AND ANY OTHER COMMAND !!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    int size = 0;
    for (char *t = lastWord; *t != '\0'; t++) {
        printf("\x1b[%im%c",30 + size%8, *t);
        size++;
    }
    
    printf(ANSI_COLOR_RESET"\n\n\n");


    while (1) {
            int CommandNumber = 0;

        printf("Shell> ");
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        // Remove newline character from the command
        command[strcspn(command, "\n")] = '\0';

        // Exit if "exit" command is entered
        if (strcmp(command, "exit") == 0) {
            break;
        }

        // Tokenize the command
        char* token = strtok(command, " ");
        int i = 0;
        while (token != NULL) {
            if (strcmp(token, ">") == 0) {
                // Output redirection
                token = strtok(NULL, " ");
                ChangeOutput(token); // do not forget to change output back to std
                token = strtok(NULL, " ");
            }
            else if (strcmp(token, "<") == 0) {
                // Input redirection
                token = strtok(NULL, " ");
                ChangeInput(token); // do not forget to change input back to std
                token = strtok(NULL, " ");
            }
            else if (strcmp(token, "|") == 0){
                //redirect the IO for pipes 
                args[CommandNumber][i] = NULL;
                i=0;

                CommandNumber += 1;
                token = strtok(NULL, " ");
            }
            else{
                args[CommandNumber][i] = token;
                token = strtok(NULL, " ");
                i++;
            }
            
        }
        args[CommandNumber][i] = NULL; 
        
        //change working direcotry of the parent processus so it transmit to all child after
        if(strcmp(args[0][0], "cd") == 0){
            if (args[0][1] != NULL) {
                changeDirectory(args[0][1]);
            } else {
                fprintf(stderr, "cd: Missing argument.\n");
            }
        }
        //
        else{
            executePipe(args,CommandNumber);
        }

        // redirection des sorties standar vers le shell si elles ont étés changés
        dup2(originalStdout,STDOUT_FILENO);
        dup2(originalStdout, STDIN_FILENO);
        
    }

    return 0;
}
