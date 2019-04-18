//PROGRAM 1: Simple shell

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

struct Command {
    char *user_input;
    int max_command_size;
    char* commands[16];
    bool inputFlag;
    bool outputFlag;
    char* file;
    bool pipeFlag;
    int numPipe;
    char *pipeCommands[30];
    bool backgroundFlag;
};

struct Command parseCommand(struct Command cmd, char* original);
struct Command parsePipe(struct Command cmd, char* original);
void pipeCommand(struct Command command);
bool file_exist (char *filename);
void saveString(char* value, char** matrix, int size);
void pipeInput(struct Command commandArg);
void pipeOutput(struct Command commandArg);
void closeProcess(int fdOdd[2], int fdEven[2], struct Command command, int pipeIndex);
void endingPipeCommand(int fd[2], struct Command commandArg, int pipeIndex);
void middlePipeCommand(int fd1[2], int fd2[2], struct Command commandArg, int pipeIndex);
void display_prompt(void);
void printStatusMessage(char *command, int exitcode);

int main(int argc, char *argv[])
{
    while (1) {
        //Command Initialization&Reading, max command size 512
        struct Command command;
        command.max_command_size = 512;
        command.user_input = (char *) malloc(512);
        int retval = 0;//Define return value, default 0
        display_prompt();
        fgets(command.user_input, command.max_command_size, stdin);
        //If using shell script input
        if (!isatty(STDIN_FILENO)) {
            printf("%s", command.user_input);
            fflush(stdout);
        }
        strtok(command.user_input, "\n");
        
        //Argument Processing
        //Case Pipe
        if (strstr(command.user_input, "|")) {
            pipeCommand(command);
        }
        //Not Piping
        else {
            //parse the Command
            command = parseCommand(command, command.user_input);
            
            //Builtin Commands
            if (strcmp(command.commands[0], "exit") == 0) {//exit command
                fprintf(stderr, "Bye...\n");
                exit(0);
            } else if (strcmp(command.commands[0], "cd") == 0) {//cd command
                int status = chdir(command.commands[1]);
                if(status != 0) {
                    fprintf(stderr, "Error: no such directory\n");
                    printStatusMessage(command.user_input, 1);
                } else {
                    printStatusMessage(command.user_input, 0);
                }
            } else if (strcmp(command.commands[0], "pwd") == 0) {//pwd command
                char *dir = getcwd(NULL, 4096);
                fprintf(stdout, "%s\n", dir);
                printStatusMessage(command.user_input, 0);
            } else {
                //Case Input Redirection
                if (command.inputFlag == true) {
                    int object_file;
                    if (command.file != NULL) {
                        object_file = open(command.file, O_RDWR);
                        if (file_exist(command.file) == false) {
                            fprintf(stderr, "Error: cannot open input file\n");
                        } else {
                            pid_t pid;
                            pid = fork();
                            if (pid == 0) {
                                dup2(object_file, STDIN_FILENO);
                                close(object_file);
                                execvp(command.commands[0], command.commands);
                                exit(1);
                            } else if (pid > 0) {
                                waitpid(-1, &retval, 0);
                                printStatusMessage(command.user_input, WEXITSTATUS(retval));
                            } else {
                                perror("fork");
                                exit(1);
                            }
                        }
                    } else {
                        fprintf(stderr, "Error: no input file\n");
                    }
                }
                //Case Output Redirection
                else if (command.outputFlag == true) {
                    int object_file;
                    if (command.file != NULL) {
                        object_file = open(command.file, O_RDWR | O_TRUNC | O_CREAT, 0666);
                        if (!file_exist(command.file)) {
                            fprintf(stderr, "Error: cannot open output file\n");
                        } else {
                            pid_t pid;
                            pid = fork();
                            if (pid == 0) {
                                dup2(object_file, STDOUT_FILENO);
                                close(object_file);
                                execvp(command.commands[0], command.commands);
                                exit(1);
                            } else if (pid > 0) {
                                waitpid(-1, &retval, 0);
                                printStatusMessage(command.user_input, WEXITSTATUS(retval));
                            } else {
                                perror("fork");
                                exit(1);
                            }
                        }
                    } else {
                        fprintf(stderr, "Error: no output file\n");
                    }
                }
                //Other Commands
                else {
                    pid_t pid;
                    pid = fork();
                    if (pid == 0) {
                        execvp(command.commands[0], command.commands);
                        fprintf(stderr, "Error: command not found\n");
                        exit(1);
                    } else if (pid > 0) {
                        if(command.backgroundFlag == true){
                            waitpid(pid, &retval,0);
                            printStatusMessage(command.user_input, WEXITSTATUS(retval));
                        } else {
                            waitpid(-1, &retval, 0);
                            printStatusMessage(command.user_input, WEXITSTATUS(retval));
                        }
                    } else {
                        perror("fork");
                        exit(1);
                    }
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

// save a value to matrix
void saveString(char* value, char** matrix, int size)
{
    (*matrix) = (char*) malloc(size);
    strcpy((*matrix), value);
}

//Check if file exists
bool file_exist (char *filename)
{
    struct stat buffer;
    return (stat (filename, &buffer) == 0);
}

struct Command parseCommand(struct Command cmd, char* inputCommand)
{
    //Initialization for command
    for (int i = 0; i < 16; ++i){
        cmd.commands[i] = NULL;
    }
    cmd.inputFlag = false;
    cmd.outputFlag = false;
    cmd.pipeFlag = false;
    cmd.file = NULL;
    cmd.backgroundFlag = false;
    
    //Temporary copy for command
    char inputCommandCopy[512];
    strcpy(inputCommandCopy, inputCommand);
    
    //Process argument by argument
    char* argument;
    argument = strtok(inputCommandCopy, " ");
    
    //Index in Commands array
    int index = 0;
    //Parse the arguments
    while (argument != NULL) {
        //Detected <, input redirection
        if (strstr(argument, "<")) {
            if(strlen(argument) == 1){
                cmd.inputFlag = true;
            } else {
                char *subargument = NULL;
                char *subargumentRest = subargument;
                subargument = strtok_r(argument, "<", &subargumentRest);
                int length = 1 + (int)strlen(subargument);
                saveString(subargument, &cmd.commands[index], length);
                index++;
                subargumentRest = strtok_r(NULL, "<", &subargumentRest);
                if (subargument != NULL) {
                    cmd.file = subargument;
                    int length = 1 + (int)strlen(subargument);
                    saveString(subargument, &cmd.commands[index], length);
                    index++;
                }
            }
        } else if (strstr(argument, ">")) {
            //Detected >, output redirection
            if (strlen(argument) == 1) {
                cmd.outputFlag = true;
            } else {
                char *subargument = NULL;
                char *subargumentRest = subargument;
                subargument = strtok_r(argument, ">", &subargumentRest);
                int size = 1 + (int)strlen(subargument);
                saveString(subargument, &cmd.commands[index], size);
                index++;
                subargument = strtok_r(NULL, ">", &subargumentRest);
                if (subargument != NULL) {
                    cmd.file = subargument;
                }
            }
        } else if(strstr(argument, "&")) {
            //Detected &, Background process
            cmd.backgroundFlag = true;
            char *subargument = NULL;
            char *subargumentRest = subargument;
            subargument = strtok_r(argument, "&", &subargumentRest);
            int size = 1 + (int)strlen(subargument);
            saveString(subargument, &cmd.commands[index], size);
            index++;
            subargument = strtok_r(NULL, "&", &subargumentRest);
            if (subargument != NULL) {
                cmd.file = subargument;
            }
        } else {
            //If there was file processing
            if (cmd.inputFlag == true || cmd.outputFlag == true) {
                cmd.file = argument;
            } else {
                int length = 1 + (int)strlen(argument);
                saveString(argument, &cmd.commands[index], length);
                index++;
            }
        }
        argument = strtok(NULL, " ");
    }
    return cmd;
}

//Detected |, Piping
struct Command parsePipe(struct Command cmd, char* initial_command)
{
    //seperate commands
    for(int i = 0; i < 30; ++i){
        cmd.pipeCommands[i] = NULL;
    }
    char* temp_command;
    char command[512];
    int index = 0;
    
    //first command
    cmd.pipeFlag = false;
    cmd.numPipe = -1;
    strcpy(command, initial_command);
    temp_command = strtok(command, "|");
    if(strstr(initial_command, "|")){
        cmd.pipeFlag = true;
    }
    //rest of commands
    while(temp_command != NULL)
    {
        cmd.numPipe++;
        int length = 1 + (int)strlen(temp_command);
        saveString(temp_command, &cmd.pipeCommands[index], length);
        index++;
        temp_command = strtok(NULL, "|");
    }
    return cmd;
}

//Pipe Input Redirection
void pipeInput(struct Command commandArg){
    int object;
    object = open(commandArg.file, O_RDWR);
    dup2(object, STDIN_FILENO);
    close(object);
    execvp(commandArg.commands[0], commandArg.commands);
    exit(1);
}

//Pipe Output Redirection
void pipeOutput(struct Command commandArg){
    int object;
    object = open(commandArg.file, O_RDWR |O_CREAT | O_TRUNC, 0666);
    dup2(object, STDOUT_FILENO);
    close(object);
    execvp(commandArg.commands[0], commandArg.commands);
    exit(1);
}

void closeProcess(int pipeOdd[2], int pipeEven[2], struct Command command, int pipeIndex){
    if (pipeIndex == 0) {
        close(pipeEven[1]);
    } else if (pipeIndex == command.numPipe) {
        if ((command.numPipe + 1)  % 2 != 0) {
            close(pipeOdd[0]);
        } else {
            close(pipeEven[0]);
        }
    } else {
        if (pipeIndex % 2 != 0) {
            close(pipeEven[0]);
            close(pipeOdd[1]);
        } else {
            close(pipeOdd[0]);
            close(pipeEven[1]);
        }
    }
}

void endingPipeCommand(int fd[2], struct Command PipeCommand, int pipeIndex) {
    if (pipeIndex == 0) {
        dup2(fd[1], STDOUT_FILENO);
    } else {
        dup2(fd[0], STDIN_FILENO);
    }
    if (PipeCommand.inputFlag == true) {
        pipeInput(PipeCommand);
    } else if (PipeCommand.outputFlag == true) {
        pipeOutput(PipeCommand);
    } else {
        execvp(PipeCommand.commands[0], PipeCommand.commands);
        fprintf(stderr, "Error: command not found\n");
        exit(1);
    }
}

void middlePipeCommand(int fd1[2], int fd2[2], struct Command commandArg, int pipeIndex) {
    dup2(fd1[0], STDIN_FILENO);
    dup2(fd2[1], STDOUT_FILENO);
    if (commandArg.inputFlag == true) {
        pipeInput(commandArg);
    } else if (commandArg.outputFlag == true) {
        pipeOutput(commandArg);
    } else {
        execvp(commandArg.commands[0], commandArg.commands);
        fprintf(stderr, "Error: command not found\n");
        exit(1);
    }
}

void pipeCommand(struct Command command) {
    //parse the commands
    command = parsePipe(command, command.user_input);
    if (strstr(command.pipeCommands[0], ">")) {
        fprintf(stderr,"Error: mislocated output redirection\n");
    } else if (strstr(command.pipeCommands[command.numPipe], "<")) {
        fprintf(stderr,"Error: mislocated input redirection\n");
    } else {
        int temp_retval = 0;
        int pipeIndex = 0;
        int pipeOdd[2];
        int pipeEven[2];
        int retvalues[command.numPipe + 1];
        int retvalIndex = 0;
        
        while (command.pipeCommands[pipeIndex] != NULL) {
            if (pipeIndex % 2 != 0){
                pipe(pipeOdd);
            } else {
                pipe(pipeEven);
            }
            
            struct Command PipeCommand;
            PipeCommand.max_command_size = 512;
            PipeCommand.user_input = (char *) malloc(512);
            PipeCommand = parseCommand(PipeCommand, command.pipeCommands[pipeIndex]);
            
            int pid = fork();
            if(pid == 0){
                if(pipeIndex == 0) {
                    endingPipeCommand(pipeEven, PipeCommand, pipeIndex);
                } else if(pipeIndex == command.numPipe) {
                    if ((command.numPipe +1) % 2 != 0) {
                        endingPipeCommand(pipeOdd, PipeCommand, pipeIndex);
                    } else {
                        endingPipeCommand(pipeEven, PipeCommand, pipeIndex);
                    }
                } else {
                    if (pipeIndex % 2 != 0) {
                        middlePipeCommand(pipeEven, pipeOdd, PipeCommand, pipeIndex);
                    } else {
                        middlePipeCommand(pipeOdd, pipeEven, PipeCommand, pipeIndex);
                    }
                }
            } else if (pid > 0) {
                waitpid(-1, &temp_retval, 0);
            } else {
                perror("fork");
                exit(1);
            }
            closeProcess(pipeOdd, pipeEven, PipeCommand, pipeIndex);
            waitpid(pid,NULL,0);
            pipeIndex++;
            retvalues[retvalIndex] = temp_retval;
            retvalIndex++;
            free(PipeCommand.user_input);
        }
        //printAllStatusMessages
        fprintf(stderr, "+ completed '%s'", command.user_input);
        for (int index = 0; index < sizeof(retvalues); index++) {
            fprintf(stderr, "[%d]", WEXITSTATUS(retvalues[index]));
        }
        fprintf(stderr, "\n");
    }
}

void display_prompt(){
    fprintf(stdout, "sshell$ ");
    fflush(stdout);
}

void printStatusMessage(char *command, int exitcode)
{
    fprintf(stderr, "+ completed '%s' [%d]\n", command, exitcode);
}
