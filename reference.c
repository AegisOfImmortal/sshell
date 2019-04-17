#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct UsersInput{
    char *input;
    char *arguments[16];
}UsersInput;

void RemoveWhiteSpace(UsersInput *temp) {
  char *buffer = temp->arguments[1];
  int i,count=0;
  for (size_t i = 0; buffer[i]; i++) {
    if (buffer[i] != ' ') {
      buffer[count] = buffer[i];
      count++;
    }
  }
  buffer[count] = '\0';
}

void SpaceTok(UsersInput *Tokinput, char* delim) {
  int count = 1;
  char *parsedInput = strtok(Tokinput->input, delim);
  Tokinput->arguments[0] = parsedInput;

  //store all arguments in cmd struct
  while (parsedInput != NULL) {
    if (count < 16) {
      //printf("parsedInput:%s\n",parsedInput );
      parsedInput = strtok(NULL, delim);
      Tokinput->arguments[count] = parsedInput;
      count++;
    }
  }
}

void redirection(UsersInput *redirect, char *direction) {
  char delim[] = " ";
  int FileCheck = 0;
  SpaceTok(redirect,direction);
  struct UsersInput Commands;
  Commands.input = (char *) malloc(1 + strlen(redirect->arguments[0]));
  strcpy(Commands.input,redirect->arguments[0]);
  SpaceTok(&Commands, delim);
  RemoveWhiteSpace(redirect);

  if (strcmp(direction, "<") == 0) {
    FileCheck = open(redirect->arguments[1], O_RDONLY, 0644);
  }
  else if (strcmp(direction, ">") == 0) {
    FileCheck = open(redirect->arguments[1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
  }
  if(FileCheck < 0){
    perror("Error");
  }
  if (strcmp(direction, "<") == 0) {
    dup2(FileCheck, 0);
  }
  else if (strcmp(direction, ">") == 0) {
    dup2(FileCheck, 1);
  }

  close(FileCheck);
  //char *args[] = {"grep","toto", NULL };
  if (FileCheck != -1) {
    execvp(Commands.arguments[0], Commands.arguments);
    perror("execvp error");
  }
}

//count how many pipe arguments there are in the input
int countPipes(char *input, int len){
  int count = 0;
  for(int i = 0; i < len; i++){
    if(input[i]== '|'){
      count += 1;
    }
  }
  if (count >= 1) {
    count +=1;
  }
  return count;
}

void piping(char **commands, int pipeCount, int currPipePhase)
{
  char delim[] = " ";
  char IRdelim[] = "<";
  char ORdelim[] = ">";
  int status;
while (currPipePhase < pipeCount) {
  char *getridofspace = commands[currPipePhase];
  int begin = 0;
  int end = strlen(getridofspace) - 1;
  int i;
  char space = ' ';
  while (getridofspace[begin] == ' ')
      begin++;

  while ((end >= begin) && (getridofspace[end] == ' '))
      end--;

  // Shift all characters back to the start of the string array.
  for (i = begin; i <= end; i++)
      getridofspace[i - begin] = getridofspace[i];

  getridofspace[i - begin] = '\0'; // Null terminate string.

  //if in last process
  if(currPipePhase == pipeCount - 1)
  {
    struct UsersInput space;
    space.input = (char *) malloc(1 + strlen(commands[currPipePhase]));
    strcpy(space.input, commands[currPipePhase]);
    //check if it is not a redirection
    //if redirection
    if (strstr(commands[currPipePhase], "<") != NULL) {
      redirection(&space,IRdelim);
    }
    else
    if (strstr(commands[currPipePhase], ">") != NULL) {
      redirection(&space,ORdelim);
    }
    else
    {
      SpaceTok(&space,delim);
      execvp(space.arguments[0],space.arguments);
      perror("execvp error");
    }
    //end recursion and return
  }
  //if on other pipe processes
  if(currPipePhase < pipeCount)
  {
    int fd[2];
    pid_t pid;

    //check for pipe error
    if(pipe(fd) < 0)
    {
      perror("pipe");
    }

    //check for forking error
    if( (pid = fork()) < 0 )
    {
      perror("fork");
    }

    //if parent
    if( pid != 0)
    {
      //close terminal FD (file descriptor) and replace with the 'write' FD
      dup2(fd[1],1);
      //close the 'reading' port
      close(fd[0]);

      //check if it is not a redirection
      struct UsersInput space;

      space.input = (char *) malloc(1 + strlen(commands[currPipePhase]));
      strcpy(space.input, commands[currPipePhase]);
        //if redirection
        if (strstr(commands[currPipePhase], "<") != NULL) {
          redirection(&space,IRdelim);
        }
        else
        if (strstr(commands[currPipePhase], ">") != NULL) {
          redirection(&space,ORdelim);
        }else
          {
            SpaceTok(&space,delim);
            execvp(space.arguments[0],space.arguments);
            perror("execvp error");
          }
      waitpid(pid, &status, 0); // wait for child process to finish
      fprintf(stderr, "+ completed 'Checking:' [%d]\n",WEXITSTATUS(status));

    }
    //if child
    else
    {
      //if not on the last piping process
      if(currPipePhase != pipeCount-1){
        //close the keyboard FD (file descriptor) and replace with the 'read' RD
        dup2(fd[0],0);
        close(fd[1]);
      }
      //close the 'write' FD
      close(fd[1]);

      //increment the traker of the current pipe phase we are on
      currPipePhase+= 1;
}

      //do next pipe process
      //piping(commands, pipeCount, currPipePhase);
    }
  }
}

int main(int argc, char *argv[])
{
  struct UsersInput entered;
  struct UsersInput IRstruct;
  struct UsersInput InputArray;
  size_t bufsize = 32;
  int status;
  pid_t pid;
  char delim[] = " ";
  char IRdelim[] = "<";
  char ORdelim[] = ">";
  char Pipedelim[] = "|";
  char *inputCpy,*IRtok;
  int bul = 0;
  int numOfPipes = 0;
  int cderr = 1;

  while (1) {
    bul = 0;
    cderr = 1;
    entered.input = (char *)malloc(bufsize * sizeof(char));
    entered.input = NULL;
    printf("sshell$ ");
    getline(&entered.input,&bufsize,stdin);
    if (!isatty(STDIN_FILENO)) {
        printf("%s", entered.input);
        fflush(stdout);
    }
    if (entered.input[0] == '\n') {
      continue;
    }
    else{
      char *spot;
      if ((spot=strchr(entered.input, '\n')) != NULL)
        *spot = '\0';
    }

    //get the number of pipes the user inputted
    numOfPipes = countPipes(entered.input, strlen(entered.input));

    //allocate space for a copy userInput
    inputCpy = (char *) malloc(1 + strlen(entered.input));
    IRstruct.input = (char *) malloc(1 + strlen(entered.input));
    InputArray.input = (char *) malloc(1 + strlen(entered.input));
    strcpy(inputCpy,entered.input);
    strcpy(IRstruct.input,entered.input);
    strcpy(InputArray.input, entered.input);
    SpaceTok(&entered, delim);
    SpaceTok(&InputArray,Pipedelim);

    // int i = 0;
    // printf("[" );
    // while (InputArray.arguments[i] != NULL) {
    //   if (i < sizeof(InputArray.arguments))
    //       printf("%s, ", InputArray.arguments[i]);
    //       i++;
    // }printf("]\n" );

    //check to see if command is 'cd'
    if (strcmp("cd",entered.arguments[0]) == 0){
      cderr = chdir(entered.arguments[1]);
      bul = 1;
      if (cderr == -1) {
        fprintf(stderr,"Error: no such directory\n");
        continue;
      }
      fprintf(stderr, "+ completed '%s' [%d]\n",inputCpy,WEXITSTATUS(status));
    }
    //check to see if command is 'exit'
    if (strcmp("exit",entered.arguments[0]) == 0){
      fprintf(stderr, "Bye...\n");
      exit(0);
    }
    //-------------------shell skeleton-------------------
    //fork a process (create parent and child)

    pid = fork();
    //parent
    if (pid > 0) {
      waitpid(-1, &status, 0);
      fprintf(stderr, "+ completed '%s' [%d]\n",inputCpy,WEXITSTATUS(status));
    }
    //child
    else if (pid == 0) {
      if (strcmp(entered.input,"\n") == 0 || bul == 1) {
          continue;
      }
      //if there is piping
      else if(numOfPipes != 0){
        //get input and parse through
        piping(InputArray.arguments ,numOfPipes, 0);
      }
      else{
        if (strstr(inputCpy, "<") != NULL) {
          redirection(&IRstruct,IRdelim);

        }
        else
        if (strstr(inputCpy, ">") != NULL) {
          redirection(&IRstruct,ORdelim);
        }
        else{
        execvp(entered.arguments[0],entered.arguments);
        perror("execvp");
        }
      }
    }
    //fork failed
    else
    {
      printf("Fork Failed\n");
      exit(1);
    }
  }
}