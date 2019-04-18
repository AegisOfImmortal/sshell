sshell: sshell.c
	gcc sshell.c -Wall -Werror -o sshell.out
clean:
	rm *.out
