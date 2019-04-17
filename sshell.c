#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

struct CommandInput {
    char* command;
    char** parsed_command;
};

char *read_command(int max_command_size){
    char *command;
    command = malloc(sizeof(char) * max_command_size);
    if (command == NULL) {
        fprintf(stdout, "Failed to allocate command");
        exit(EXIT_FAILURE);
    }
    int input_character;
    size_t command_length = 0;
    while (EOF != (input_character = fgetc(stdin)) && input_character != '\n')
    {
        command[command_length + 1] = (char) input_character;
        if (command_length == max_command_size)
        {
            fprintf(stdout, "command exceeds 512");
            exit(EXIT_FAILURE);
        }
    }
    command[command_length + 1] = '\0';
    return command;
}

void display_prompt(){
    fprintf(stdout, "sshell$ ");
}

void excecute(struct CommandInput user_command){
    int status;
    if (fork() == 0) {
        /* Child */
        //execvp();
        perror("execvp");
        exit(1);
    }else{
        /* Parent */
        waitpid(-1, &status, 0);
        printf("Child exited with status: %d\n", WEXITSTATUS(status));
    }
}

struct CommandInput parse_command(char* command){
    struct CommandInput user_command;
    return user_command;
}

int main(int argc, char *argv[])
{
    struct CommandInput user_command;
    /*user_command.command = (char*)malloc(512 * sizeof(char));
     user_command.parsed_command = (char**)malloc(512 * sizeof(char*));
     for (int i = 0; i < 512; i++) {
     user_command.parsed_command[i] = (char*)malloc(10 * sizeof(char));
     }*/
    while (1) {
        display_prompt();
        char *command = read_command(512);
        user_command = parse_command(command);
        excecute(user_command);
    }
    return EXIT_SUCCESS;
}
