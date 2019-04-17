#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct CommandInput {
    char * command;
    char * parsed_command;
    char ** commands;
    int length;
};

void executeBuiltin(struct CommandInput user_command);
void executeCommand(struct CommandInput user_command);
void printStatusMessage(char *command, int exitcode);

int isBuiltin(char *command)
{
    if (strcmp("exit", command) == 0 || strcmp("cd", command) == 0 || strcmp("pwd", command) == 0)
    {
        return 1;
    }
    return 0;
}


void read_command(struct CommandInput user_command){
    fgets(user_command.command, 512, stdin);
    if (!isatty(STDIN_FILENO)) {
        printf("%s", user_command.command);
        fflush(stdout);
    }
    user_command.length = (int)strlen(user_command.command);
    int filler = user_command.length - 1;
    while(filler < 512) {
        user_command.command[filler++] = '\0';
    }
    strcpy(user_command.parsed_command, user_command.command);
}

void display_prompt(){
    fprintf(stdout, "sshell$ ");
}

void excecute(struct CommandInput user_command){
    if(isBuiltin(user_command.commands[0])) {
        executeBuiltin(user_command);
    } else {
        executeCommand(user_command);
    }
}

void parse_command(struct CommandInput user_command){
    char *arg = strtok(user_command.parsed_command, " ");
    int i = 0;
    while (arg != NULL){
        user_command.commands[i++] = arg;
        arg = strtok(NULL, " ");
    }
    while(i < 16) {
        user_command.commands[i++] = '\0';
    }
}

void executeBuiltin(struct CommandInput user_command)
{
    
    if(strcmp("exit", user_command.commands[0]) == 0) {
        fprintf(stderr, "Bye...\n");
        exit(0);
    } else if(strcmp("pwd", user_command.commands[0]) == 0) {
        char *dir = getcwd(NULL, 4096);
        fprintf(stdout, "%s\n", dir);
        
        printStatusMessage(user_command.command, 0);
    } else if(strcmp("cd", user_command.commands[0]) == 0) {
        int status = chdir(user_command.commands[1]);
        
        if(status != 0) {
            fprintf(stderr, "Error: no such directory\n");
            printStatusMessage(user_command.command, 1);
        } else {
            printStatusMessage(user_command.command, 0);
        }
        
    }
}

void executeCommand(struct CommandInput user_command)
{
    // Create a new child process to run the command
    int pid = fork();
    
    if(pid == 0) {
        // If the code is running in the original process, execute the command
        execvp(user_command.commands[0], user_command.commands);
        
        // Execution should never reach this point unless command is not found
        fprintf(stderr, "Error: command %s not found\n",user_command.commands[0]);
        
        // Return code of child process should be 1 if the command is not found
        exit(1);
        
    } else if(pid > 0) {
        // If code is running in the child process, wait for the command to finish
        wait(&pid);
        printStatusMessage(user_command.command, WEXITSTATUS(pid));
    } else {
        // There has been an error with the fork
        fprintf(stderr, "Error: internal system error");
        exit(1);
    }
}

void printStatusMessage(char *command, int exitcode)
{
    fprintf(stderr, "+ completed '%s' [%d]\n", command, exitcode);
}

int main(int argc, char *argv[])
{
    struct CommandInput *user_command = (struct CommandInput*)malloc(sizeof(struct CommandInput));
    user_command->command = malloc (512 * sizeof (char));
    user_command->parsed_command = malloc (512 * sizeof (char));
    user_command->commands = (char**) malloc(16 * sizeof(char*));
    for (int i = 0; i<16; i++) {
        user_command->commands[i] = (char*) malloc(513*sizeof(char));
    }
    while (1) {
        display_prompt();
        read_command(*user_command);
        parse_command(*user_command);
        excecute(*user_command);
    }
    return EXIT_SUCCESS;
}

