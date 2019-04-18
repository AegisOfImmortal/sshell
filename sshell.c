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


struct CommandInput {
    char * command;
    char * parsed_command;
    char ** commands;
    int length;
    bool inputFlag;
    bool outputFlag;
    bool pipeFlag;
    int numPipe;
    char *pipeCommands[40];
    char * file;
    bool backgroundFlag;
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

void copyTemp(char* temp, char** element, int size)
{
        (*element) = (char*) malloc(size);
        strcpy((*element), temp);
}

int file_exist (char *filename)
{
	struct stat buffer;
	return (stat (filename, &buffer) == 0);
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
	if (strstr(user_command.command, "&") && (strlen(user_command.command) == 1)) {
		fprintf(stderr, "Error: invalid command line\n");
	}else if (strstr(user_command.command, "|")) {
		pipeCommand(user_command); 
	}else{
		user_command.inputFlag = false;
		user_command.outputFlag = false;
		user_command.pipeFlag = false;
		user_command.file = NULL; 
		user_command.backgroundFlag = false;

		char *commandToken = strtok(user_command.parsed_command, " ");
		int i = 0;
		while (arg != NULL){
			if (strstr(commandToken, "<")) {
				if(strlen(commandToken) == 1){
					user_command.inputFlag = true;
				}else {
					//Set delimiter as input arrow and parse for subtokens
					char inputArrow = "<";
					char *subCommandToken;
					char *subCommandTokenSavePtr = NULL; 	

					//Strtok first time to get string token before <
					subCommandToken = strtok_r(commandToken, inputArrow, &subCommandTokenSavePtr);

					//Get length of subCommandToken in order to store into user_command args array through copyTemp
					int size = 1 + strlen(subCommandToken);
					copyTemp(subCommandToken, &user_command.commands[i],  size);
					++i;

					//Perform strtok the second time to see if any string token after <, this would be the name of the file 
					subCommandToken = strtok_r(NULL, inputArrow, &subCommandTokenSavePtr);
					if (subCommandToken != NULL) {
						user_command.file = subCommandToken;
						int size = 1 + strlen(subCommandToken);
						copyTemp(subCommandToken, &user_command.commands[i], size);
						++i;
					}
					user_command.inputFlag = true;
				}
			} else if (strstr(commandToken, ">")) {
				if (strlen(commandToken) == 1) {
					user_command.outputFlag = true;
				} else {
					char outputArrow[1] = ">";
					char *subCommandToken;
					char *subCommandTokenSavePtr = NULL;
					subCommandToken = strtok_r(commandToken, outputArrow, &subCommandTokenSavePtr);
					int size = 1 + strlen(subCommandToken);
					copyTemp(subCommandToken, &user_command.commands[i], size);
					++i;
					subCommandToken = strtok_r(NULL, outputArrow, &subCommandTokenSavePtr);
					if (subCommandToken != NULL) { 
						user_command.file = subCommandToken; 
					}
					user_command.outputFlag = true;
                }
    		} else if(strstr(commandToken, "&")) {
    			char bg[1] = "&";
    			char *subCommandToken;
    			char *subCommandTokenSavePtr = NULL;
    			subCommandToken = strtok_r(commandToken, bg, &subCommandTokenSavePtr);
    			int size = 1 + strlen(subCommandToken);
    			copyTemp(subCommandToken, &user_command.commands[i], size);
    			++i;
    			subCommandToken = strtok_r(NULL, bg, &subCommandTokenSavePtr);
    			if (subCommandToken != NULL) {
    				user_command.file = subCommandToken;
    			}
    			user_command.backgroundFlag = true;
            } else {
				if (user_command.inputFlag == true || user_command.outputFlag == true) {
					user_command.file = commandToken;
			        	if (user_command.inputFlag) {
						int size = 1 + strlen(commandToken);
						copyTemp(commandToken, &user_command.commands[i], size);
						++i; 
					}	
				} else {	      
					int size = 1 + strlen(commandToken);
					copyTemp(commandToken, &user_command.commands[i], size);
					++i; 
				}
			}
		}
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
	if (user_command.inputFlag == true) {
		int fd;
		if (user_command.file != NULL) {
			fd = open(user_command.file, O_RDWR);
			if (!file_exist(user_command.file)) {
				fprintf(stderr, "Error: cannot open input file\n"); 
			} else {
				pid_t pid;
				pid = fork();
				if (pid == 0) {
					dup2(fd, STDIN_FILENO);
					close(fd);
					execvp(user_command.commands[0], user_command.commands);
					exit(1);
				} else if (pid > 0) {
					waitpid(-1, &retval, 0);
					fprintf(stderr, "+ completed '%s' [%d]\n", user_command.command, WEXITSTATUS(retval));
				} else {
					perror("fork");
					exit(1);
				}
			}
		} else {
			fprintf(stderr, "Error: no input file\n");
		}
	} else if (user_command.outputFlag == true) {
		int fd;
		if (user_command.file != NULL) {
			fd = open(user_command.file, O_RDWR | O_TRUNC | O_CREAT, 0666);
			if (!file_exist(user_command.file)) {
				fprintf(stderr, "Error: cannot open ouput file\n");
			} else {
				pid_t pid;
				pid = fork();
				if (pid == 0) {
					dup2(fd, STDOUT_FILENO);
					close(fd);
					execvp(user_command.commands[0], user_command.commands);
					exit(1);
				} else if (pid > 0) {
					waitpid(-1, &retval, 0);
					fprintf(stderr, "+ completed '%s' [%d]\n", user_command.command, WEXITSTATUS(retval));
				} else {
					perror("fork");
					exit(1);
				}
			}
		} else {
			fprintf(stderr, "Error: no output file\n");
		}
	} else {
		pid_t pid; 
		pid = fork(); 
		if (pid == 0) {
			execvp(user_command.commands[0], user_command.commands);
			fprintf(stderr, "Error: command not found\n"); 
			exit(1); 
		} else if (pid > 0) {
			if(user_command.backgroundFlag == true){
				waitpid(pid, &retval,0); 
				fprintf(stderr, "+ completed '%s' [%d]\n", user_command.command, WEXITSTATUS(retval));
			} else {
				waitpid(-1, &retval, 0); 
				fprintf(stderr, "+ completed '%s' [%d]\n", user_command.command, WEXITSTATUS(retval)); 
			}
		} else {
			perror("fork");
			exit(1);
		}
	}
}

void printStatusMessage(char *command, int exitcode)
{
    fprintf(stderr, "+ completed '%s' [%d]\n", command, exitcode);
}

struct CommandInput splitPipe(struct CommandInput cmd, char* original)
{
        //PARSING COMMANDS
	for(int i = 0; i < 16; ++i)
                cmd.pipeCommands[i] = NULL;

        const char delim = "|";
        char* temp;
        char string[512];
        int i = 0;

        cmd.pipeFlag = false;
        cmd.numPipe = -1;
	
	//PARSING
        strcpy(string, original);
        temp = strtok(string, delim); //get the program: ls, date, etc.

        if(strstr(original, "|")){
                cmd.pipeFlag = true;
        }

	//Go through each token in the string separated by |
        while(temp != NULL)
        {
        	cmd.numPipe = cmd.numPipe + 1;
		int size = 1 + strlen(temp);
		copyTemp(temp, &cmd.pipeCommands[i], size);
                ++i;
                temp = strtok(NULL, delim); //get the next argument
        }
        return cmd;
}

/* In case valid input redirection appears in pipe command, perform the following tasks */
void pipeInputRedir(struct CommandInput commandArg){
	int fd;
        fd = open(commandArg.file, O_RDWR);
	dup2(fd, STDIN_FILENO);
	close(fd);
	execvp(commandArg.commands[0], commandArg.commands);
	exit(1);
}

/* In case valid output redirection appears in pipe command, perform the following tasks */
void pipeOutputRedir(struct CommandInput commandArg){
	int fd;
	fd = open(commandArg.file, O_RDWR |O_CREAT | O_TRUNC, 0666);                       
    	dup2(fd, STDOUT_FILENO);
      	close(fd);                                             
       	execvp(commandArg.commands[0], commandArg.commands);                                              
       	exit(1);	
}

/* Close parent processes after fork process is complete */ 
void closeParentProcess(int fdOdd[2], int fdEven[2], struct CommandInput command, int pipeIndex){
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

         
	if (commandArg.inputFlag == true) {
         	pipeInputRedir(commandArg);
        } else if (commandArg.outputFlag == true) {
                pipeOutputRedir(commandArg);
	} else {
        	execvp(commandArg.commands[0], commandArg.commands);
        }
}

/* For middle pipe commands, perform the following tasks */
void middlePipeCommand(int fd1[2], int fd2[2], struct Command commandArg, int pipeIndex) {
	dup2(fd1[0], STDIN_FILENO);
	dup2(fd2[1],STDOUT_FILENO);
	
	if (commandArg.inputFlag == true) {
        	pipeInputRedir(commandArg);
        } else if (commandArg.outputFlag == true) {
        	pipeOutputRedir(commandArg);
        } else {
                execvp(commandArg.commands[0], commandArg.commands);
	}
}

void pipeCommand(struct CommandInput user_command) { 

	//Split the command into tokens separated by | and store in command.pipeCommands array
	user_command = splitPipe(user_command, user_command.command);
  	
	if (strstr(user_command.pipeCommands[0], ">")) {
	  	printf("Error: mislocated ouput redirection\n");  
  	} else if (strstr(user_command.pipeCommands[user_command.numPipe], "<")) {
	  	printf("Error: mislocated input redirection\n"); 
  	} else {

		int retval;
        	int pipeIndex = 0;

		int fdOdd[2];
        	int fdEven[2];

		int retvalues[user_command.numPipe + 1];
        	int retvalIndex = 0;
 		
		while (user_command.pipeCommands[pipeIndex] != NULL) {
    		if (pipeIndex % 2 != 0){
        		pipe(fdOdd);
    		} else {
        		pipe(fdEven); 
    		}	

   		 	struct CommandInput commandArg;
		 	commandArg = splitCommand(commandArg, user_command.pipeCommands[pipeIndex]);

			int pid = fork();
			if(pid == 0){
				if(pipeIndex == 0) {
					firstAndLastPipeCommand(fdEven, commandArg, pipeIndex);
					}else if(pipeIndex == command.numPipe) {
						if ((command.numPipe +1) % 2 != 0) {
							firstAndLastPipeCommand(fdOdd, commandArg, pipeIndex); 
						} else {
							firstAndLastPipeCommand(fdEven, commandArg, pipeIndex);
						}
					} else {
						if (pipeIndex % 2 != 0) {
							middlePipeCommand(fdEven, fdOdd, commandArg, pipeIndex);
						} else {
							middlePipeCommand(fdOdd, fdEven, commandArg, pipeIndex);
						}
					}
				} else if (pid > 0) {
					waitpid(-1, &retval, 0);
				}
				closeParentProcess(fdOdd, fdEven, commandArg, pipeIndex);
  				waitpid(pid,NULL,0);
  				pipeIndex++;
  				retvalues[retvalIndex] = retval;
  				retvalIndex++;
  				}//end of while

  		//FINAL PRINT STATEMENTS  
  		fprintf(stderr, "+ completed '%s'", user_command.command);
  		for (int y = 0; y < sizeof(retvalues)/sizeof(int); y++) { 
  			fprintf(stderr, "[%d]", WEXITSTATUS(retvalues[y])); 
  		}
  		fprintf(stderr, "\n");
 	}//end of else
} //end of pipeCommand


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

