#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct CommandInput {
    char* command;
    char* flag;//FIXME: change flag to a string instead of list
    char** argument;
};

int main(int argc, char *argv[])
{
    char *cmd = (char*)malloc(512 * sizeof(char));
    int retval;
    printf("sshell$ ");
    fgets(cmd, sizeof(char) * 512, stdin);
    retval = system(cmd);
    fprintf(stdout, "Return status value for '%s': %d\n", cmd, retval);
    
    return EXIT_SUCCESS;
}
