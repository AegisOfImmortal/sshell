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
    char *input_copy;
    int max_command_size;
    char* args[16];
    int maxArgCount;
    char* file;
    bool pipe;
    int numPipe;
    char *pipeCommands[40];
    bool background;
    bool inputRedir;
    bool outputRedir;
};

struct Command splitCommand(struct Command cmd, char* original);
struct Command splitPipe(struct Command cmd, char* original);
void pipeCommand(struct Command command);
int file_exist (char *filename);
void copyTemp(char* temp, char** element, int size);
void pipeInputRedir(struct Command commandArg);
void pipeOutputRedir(struct Command commandArg);
void closeParentProcess(int fdOdd[2], int fdEven[2], struct Command command, int pipeIndex);
void firstAndLastPipeCommand(int fd[2], struct Command commandArg, int pipeIndex);
void middlePipeCommand(int fd1[2], int fd2[2], struct Command commandArg, int pipeIndex);
void display_prompt(void);

int main(int argc, char *argv[])
{
    while (1) {
        //Command Initialization
        struct Command command;
        command.max_command_size = 512;
        command.user_input = (char *) malloc(512);
        command.input_copy = (char *) malloc(512);
        
        //Return value
        int retval = 0;
        
        //User input from the shell
        printf("sshell$ ");
        fflush(stdout);
        fgets(command.user_input, command.max_command_size, stdin);
        
        //TESTER CODE
        /* Print command line if we're not getting stdin from the
         * terminal */
        if (!isatty(STDIN_FILENO)) {
            printf("%s", command.user_input);
            fflush(stdout);
        }
        
        //Remove newline character at end for proper token parsing
        strtok(command.user_input, "\n");
        
        /***** PARSING ARGS *******/
        // if &
        if (strstr(command.user_input, "&") && (strlen(command.user_input) == 1)) {
            fprintf(stderr, "Error: invalid command line\n");
        }// if pipe
        else if (strstr(command.user_input, "|")) {
            //Perform separate functionality for commands with pipe
            pipeCommand(command);
        }
        else {
            //Split command into tokens
            command = splitCommand(command, command.user_input);
            
            //Take care of exit, cd, pwd and then only input redirection, output redirection and other commands
            if (strcmp(command.args[0], "exit") == 0) {
                fprintf(stderr, "Bye...\n");
                break;
            } else if (strcmp(command.args[0], "cd") == 0) {
                int cdReturnValue = chdir(command.args[1]);
                if (cdReturnValue == -1) {
                    fprintf(stderr, "Error: no such directory \n" );
                    fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, 1);
                } else {
                    fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, 0);
                }
            } else if (strcmp(command.args[0], "pwd") == 0) {
                char cwd[256];
                if (getcwd(cwd, sizeof(cwd)) == NULL){
                    fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, 1);
                } else {
                    getcwd(cwd, 256);
                    fprintf(stdout, "%s\n", cwd);
                    fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, 0);
                }
            } else {
                /*** INPUT REDIRECTION ***/
                if (command.inputRedir == true) {
                    int fd;
                    if (command.file != NULL) {
                        fd = open(command.file, O_RDWR);
                        if (!file_exist(command.file)) {
                            fprintf(stderr, "Error: cannot open input file\n");
                        } else {
                            pid_t pid;
                            pid = fork();
                            if (pid == 0) {
                                dup2(fd, STDIN_FILENO);
                                close(fd);
                                execvp(command.args[0], command.args);
                                exit(1);
                            } else if (pid > 0) {
                                waitpid(-1, &retval, 0);
                                fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, WEXITSTATUS(retval));
                            } else {
                                perror("fork");
                                exit(1);
                            }
                        }
                    } else {
                        fprintf(stderr, "Error: no input file\n");
                    }
                    /*** OUTPUT REDIRECTION ***/
                } else if (command.outputRedir == true) {
                    int fd;
                    if (command.file != NULL) {
                        fd = open(command.file, O_RDWR | O_TRUNC | O_CREAT, 0666);
                        if (!file_exist(command.file)) {
                            fprintf(stderr, "Error: cannot open ouput file\n");
                        } else {
                            pid_t pid;
                            pid = fork();
                            if (pid == 0) {
                                dup2(fd, STDOUT_FILENO);
                                close(fd);
                                execvp(command.args[0], command.args);
                                exit(1);
                            } else if (pid > 0) {
                                waitpid(-1, &retval, 0);
                                fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, WEXITSTATUS(retval));
                            } else {
                                perror("fork");
                                exit(1);
                            }
                        }
                    } else {
                        fprintf(stderr, "Error: no output file\n");
                    }
                    
                    /*** EVERY OTHER COMMAND ***/
                } else {
                    pid_t pid;
                    pid = fork();
                    if (pid == 0) {
                        execvp(command.args[0], command.args);
                        fprintf(stderr, "Error: command not found\n");
                        exit(1);
                    } else if (pid > 0) {
                        if(command.background == true){
                            waitpid(pid, &retval,0);
                            fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, WEXITSTATUS(retval));
                        } else {
                            waitpid(-1, &retval, 0);
                            fprintf(stderr, "+ completed '%s' [%d]\n", command.user_input, WEXITSTATUS(retval));
                        }
                    } else {
                        perror("fork");
                        exit(1);
                    }
                }
            }
        }
    }//end of while
    
    return EXIT_SUCCESS;
}//end of main

/* Copy a string value */
void copyTemp(char* temp, char** element, int size)
{
    (*element) = (char*) malloc(size);
    strcpy((*element), temp);
}

/* Checks if file exists helper function, referred from stack overflow: https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform */
int file_exist (char *filename)
{
    struct stat buffer;
    return (stat (filename, &buffer) == 0);
}

struct Command splitCommand(struct Command cmd, char* inputCommand)
{
    //Set array which will store each of the string tokens present in the inputCommand string to NULL
    for (int i = 0; i < 16; ++i)
        cmd.args[i] = NULL;
        
        //VARS INITIALIZATION
        cmd.inputRedir = false;
        cmd.outputRedir = false;
        cmd.pipe = false;
        cmd.file = NULL;
        cmd.background = false;
        
        //Create a copy of input command in case we want to debug using input command later
        char inputCommandCopy[512];
    strcpy(inputCommandCopy, inputCommand);
    
    char* commandToken;
    commandToken = strtok(inputCommandCopy, " ");
    
    //Current index of the cmd.args array which stores each of the string tokens
    int i = 0;
    while (commandToken != NULL) {
        //INPUT REDIRECTION
        if (strstr(commandToken, "<")) {
            //Automatically set input redirection value to true if just an < appears
            if(strlen(commandToken) == 1){
                cmd.inputRedir = true;
                //Or else parse the token to get the string token before or after < (can be on both sides as well)
            } else {
                //Set delimiter as input arrow and parse for subtokens
                char inputArrow[1] = "<";
                char *subCommandToken;
                char *subCommandTokenSavePtr = NULL;
                
                //Strtok first time to get string token before <
                subCommandToken = strtok_r(commandToken, inputArrow, &subCommandTokenSavePtr);
                
                //Get length of subCommandToken in order to store into cmd args array through copyTemp
                int size = 1 + (int)strlen(subCommandToken);
                copyTemp(subCommandToken, &cmd.args[i],  size);
                ++i;
                
                //Perform strtok the second time to see if any string token after <, this would be the name of the file
                subCommandToken = strtok_r(NULL, inputArrow, &subCommandTokenSavePtr);
                if (subCommandToken != NULL) {
                    cmd.file = subCommandToken;
                    int size = 1 + (int)strlen(subCommandToken);
                    copyTemp(subCommandToken, &cmd.args[i], size);
                    ++i;
                }
                cmd.inputRedir = true;
            }
            //OUTPUT REDIRECTION
        } else if (strstr(commandToken, ">")) {
            //Automatically set output redirection value to true if just an > appears
            if (strlen(commandToken) == 1) {
                cmd.outputRedir = true;
                //Or else parse the token to get the string token before or after > (can be on both sides as well)
            } else {
                //Set delimiter as ouput arrow and parse for subtokens
                char outputArrow[1] = ">";
                char *subCommandToken;
                char *subCommandTokenSavePtr = NULL;
                
                //Strtok first time to get string token before >
                subCommandToken = strtok_r(commandToken, outputArrow, &subCommandTokenSavePtr);
                
                //Get length of subCommandToken in order to store into cmd args array through copyTemp
                int size = 1 + (int)strlen(subCommandToken);
                copyTemp(subCommandToken, &cmd.args[i], size);
                ++i;
                
                //Perform strtok the second time to see if any string token after >, this would be the name of the file
                subCommandToken = strtok_r(NULL, outputArrow, &subCommandTokenSavePtr);
                if (subCommandToken != NULL) {
                    cmd.file = subCommandToken;
                }
                cmd.outputRedir = true;
            }
            //BACKGROUND
        } else if(strstr(commandToken, "&")) {
            //Set delimiter bg symbol  and parse for subtokens
            char bg[1] = "&";
            char *subCommandToken;
            char *subCommandTokenSavePtr = NULL;
            
            //Strtok first time to get string token before &
            subCommandToken = strtok_r(commandToken, bg, &subCommandTokenSavePtr);
            
            //Get length of subCommandToken in order to store into cmd args array through copyTemp
            int size = 1 + (int)strlen(subCommandToken);
            copyTemp(subCommandToken, &cmd.args[i], size);
            ++i;
            
            //Perform strtok the second time to see if any string token after &, this would be the name of the file
            subCommandToken = strtok_r(NULL, bg, &subCommandTokenSavePtr);
            if (subCommandToken != NULL) {
                cmd.file = subCommandToken;
            }
            cmd.background = true;
        } else {
            //Store file name if input or output redirection is true
            if (cmd.inputRedir == true || cmd.outputRedir == true) {
                cmd.file = commandToken;
                if (cmd.inputRedir) {
                    int size = 1 + (int)strlen(commandToken);
                    copyTemp(commandToken, &cmd.args[i], size);
                    ++i;
                }
            } else {
                int size = 1 + (int)strlen(commandToken);
                copyTemp(commandToken, &cmd.args[i], size);
                ++i;
            }
        }
        commandToken = strtok(NULL, " ");
    }
    return cmd;
}//end of splitCommand

/* The following function splits the command input taken from the user based on the | symbol */
struct Command splitPipe(struct Command cmd, char* original)
{
    //PARSING COMMANDS
    for(int i = 0; i < 16; ++i)
        cmd.pipeCommands[i] = NULL;
        
        const char delim[2] = "|";
        char* temp;
    char string[512];
    int i = 0;
    
    cmd.pipe = false;
    cmd.numPipe = -1;
    
    //PARSING
    strcpy(string, original);
    temp = strtok(string, delim); //get the program: ls, date, etc.
    
    if(strstr(original, "|")){
        cmd.pipe = true;
    }
    
    //Go through each token in the string separated by |
    while(temp != NULL)
    {
        cmd.numPipe = cmd.numPipe + 1;
        int size = 1 + (int)strlen(temp);
        copyTemp(temp, &cmd.pipeCommands[i], size);
        ++i;
        temp = strtok(NULL, delim); //get the next argument
    }
    return cmd;
    
}

/* In case valid input redirection appears in pipe command, perform the following tasks */
void pipeInputRedir(struct Command commandArg){
    int fd;
    fd = open(commandArg.file, O_RDWR);
    dup2(fd, STDIN_FILENO);
    close(fd);
    execvp(commandArg.args[0], commandArg.args);
    exit(1);
}

/* In case valid output redirection appears in pipe command, perform the following tasks */
void pipeOutputRedir(struct Command commandArg){
    int fd;
    fd = open(commandArg.file, O_RDWR |O_CREAT | O_TRUNC, 0666);
    dup2(fd, STDOUT_FILENO);
    close(fd);
    execvp(commandArg.args[0], commandArg.args);
    exit(1);
}

/* Close parent processes after fork process is complete */
void closeParentProcess(int fdOdd[2], int fdEven[2], struct Command command, int pipeIndex){
    if (pipeIndex == 0) {
        close(fdEven[1]);
    } else if (pipeIndex == command.numPipe) {
        if ((command.numPipe + 1)  % 2 != 0) {
            close(fdOdd[0]);
        } else {
            close(fdEven[0]);
        }
    } else {
        if (pipeIndex % 2 != 0) {
            close(fdEven[0]);
            close(fdOdd[1]);
        } else {
            close(fdOdd[0]);
            close(fdEven[1]);
        }
    }
}

/* For first and last pipe commands, perform the following tasks */
void firstAndLastPipeCommand(int fd[2], struct Command commandArg, int pipeIndex) {
    if (pipeIndex == 0) {
        dup2(fd[1], STDOUT_FILENO);
    } else {
        dup2(fd[0], STDIN_FILENO);
    }
    
    
    if (commandArg.inputRedir == true) {
        pipeInputRedir(commandArg);
    } else if (commandArg.outputRedir == true) {
        pipeOutputRedir(commandArg);
    } else {
        execvp(commandArg.args[0], commandArg.args);
    }
}

/* For middle pipe commands, perform the following tasks */
void middlePipeCommand(int fd1[2], int fd2[2], struct Command commandArg, int pipeIndex) {
    dup2(fd1[0], STDIN_FILENO);
    dup2(fd2[1],STDOUT_FILENO);
    
    if (commandArg.inputRedir == true) {
        pipeInputRedir(commandArg);
    } else if (commandArg.outputRedir == true) {
        pipeOutputRedir(commandArg);
    } else {
        execvp(commandArg.args[0], commandArg.args);
    }
}

/* Pipe command tasks */
void pipeCommand(struct Command command) {
    
    //Split the command into tokens separated by | and store in command.pipeCommands array
    command = splitPipe(command, command.user_input);
    
    if (strstr(command.pipeCommands[0], ">")) {
        printf("Error: mislocated ouput redirection\n");
    } else if (strstr(command.pipeCommands[command.numPipe], "<")) {
        printf("Error: mislocated input redirection\n");
    } else {
        
        int retval = 0;
        int pipeIndex = 0;
        
        int fdOdd[2];
        int fdEven[2];
        
        int retvalues[command.numPipe + 1];
        int retvalIndex = 0;
        
        while (command.pipeCommands[pipeIndex] != NULL) {
            if (pipeIndex % 2 != 0){
                pipe(fdOdd);
            } else {
                pipe(fdEven);
            }
            
            struct Command commandArg;
            commandArg.max_command_size = 512;
            commandArg.user_input = (char *) malloc(512);
            commandArg.input_copy = (char *) malloc(512);
            commandArg = splitCommand(commandArg, command.pipeCommands[pipeIndex]);
            
            int pid = fork();
            if(pid == 0){
                if(pipeIndex == 0) {
                    firstAndLastPipeCommand(fdEven, commandArg, pipeIndex);
                    // end of pipeIndex = 0
                } else if(pipeIndex == command.numPipe) {
                    if ((command.numPipe +1) % 2 != 0) {
                        firstAndLastPipeCommand(fdOdd, commandArg, pipeIndex);
                    } else {
                        firstAndLastPipeCommand(fdEven, commandArg, pipeIndex);
                    }
                    // end of pipeIndex == command.numPipe
                } else {
                    if (pipeIndex % 2 != 0) {
                        middlePipeCommand(fdEven, fdOdd, commandArg, pipeIndex);
                    } else {
                        middlePipeCommand(fdOdd, fdEven, commandArg, pipeIndex);
                    }
                }//all other middle pid conditions
            } else if (pid > 0) {
                waitpid(-1, &retval, 0);
            }
            
            closeParentProcess(fdOdd, fdEven, commandArg, pipeIndex);
            waitpid(pid,NULL,0);
            pipeIndex++;
            retvalues[retvalIndex] = retval;
            retvalIndex++;
            free(commandArg.user_input);
            free(commandArg.input_copy);
        }//end of while
        
        //FINAL PRINT STATEMENTS
        fprintf(stderr, "+ completed '%s'", command.user_input);
        for (int y = 0; y < sizeof(retvalues)/sizeof(int); y++) {
            fprintf(stderr, "[%d]", WEXITSTATUS(retvalues[y]));
        }
        fprintf(stderr, "\n");
        
    }//end of else
} //end of pipeCommand

void display_prompt(){
    fprintf(stdout, "sshell$ ");
}
