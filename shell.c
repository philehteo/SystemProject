#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_PIPE 10

void printArguments(char* args[]) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("Argument %d: %s\n", i, args[i]);
    }
}

int execute(char* args[MAX_COMMAND_LENGTH / 2 + 1]){
    // Fork a child process
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Failed to fork a child process.\n");
        return 1;
    } else if (pid == 0) {
        // Child process
        
        execvp(args[0], args);
        
        fprintf(stderr, "Failed to execute the command.\n");
        return 1;
    } else {
        // Parent process
        wait(NULL);
    }
    return 0;
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
    char commands[MAX_PIPE][MAX_COMMAND_LENGTH];
    char* args[MAX_COMMAND_LENGTH / 2 + 1];  // +1 for NULL terminator
    int originalStdout = dup(STDOUT_FILENO);

    while (1) {
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
            }
            else{
                args[i] = token;
                token = strtok(NULL, " ");
                i++;
            }
            

        }
        args[i] = NULL;
        if(strcmp(args[0], "cd") == 0){
            if (args[1] != NULL) {
                changeDirectory(args[1]);
            } else {
                fprintf(stderr, "cd: Missing argument.\n");
            }
        }
        else{
            if(execute(args) == 1) {
                return 1;
            }
        }
        // redirection des sorties standar vers le shell si elles ont étés changés
        dup2(originalStdout,STDOUT_FILENO);
        dup2(originalStdout, STDIN_FILENO);
    }

    return 0;
}
