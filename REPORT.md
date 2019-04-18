# Phase zero
In phase zero we are trying to figure how the project as a whole works. We downloaded the professor's files and then compiled, writing a makefile all along, and found how basically the commands work via system().

# Phase one
In phase one, we added the options of using the fork-exec-wait process to run commands. In such way, by forking and creating processes, we can have the shell w-aiting for the previous job to be done while still can do other things after that.

# Phase two
In phase two, we now introduce reading commands from the user. To actually read we need a prompt telling the user to input, so display_prompt() function was added. And after that, we added simple reads from the user (fget), and grab the command. Then passing the command to phase 1's skeleton, we get a simple shell working. Also unknown commands will be reported if exec() fails.

# Phase three
In phase three, we know need method to process multi argument commands. To do this, we need a parser for commands to recognize the arguments. Right now we are just removing the white spaces, and so we strtok the input into arguments, then just run it in exec().

# Phase four
In phase four, we now need to have some builtin commands to process certain commands. The three required functions are pwd, cd and exit. As they are all just realted to the first argument, we just detect them from first argument of command. Using the respective system calls, we achieved these functions.

# Phase five
In phase five, we need some file processing for input redirection. This command is identified by the limiter "<", thus we added this into our parser to recognize it. Then the inputFlag in command (it is a struct now) will turn to true. After that, the correct exec() function is called to do the command instead of puting it directly to system.

# Phase six
In phase six, similarly to five, we just detect ">" in parser this time and then run the correspoding exec().

# Phase seven
In phase seven, we need to handle pipeline commands, and identified by "|". We first of all parse the whole command by commands using that symbol, then use the same methods in previous phases to run them. The issue here is, we have inputs and out puts passing withing pieces of pipe. To do this we gave each pipe an ID and along tracking inputs and outputs with pipeEven and pipeOdd, and therefore passing them along with the command to do what was supposed to. We make cases for the commands, in order to seperate them according to which phase's command it belongs to. And finally we close all the processes and print all the return values at once.

# Phase eight
In phase eight, we identify the background processes using symbol "&". Then we parse those commands and also make them wait for the correct amount of time using waitpid. In this method, we run processes in the background and meanwhile track their condition in the shell.
