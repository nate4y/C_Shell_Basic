/*
AUTHOR: Nate Kell
DUE DATE: 3/8/19

Fully Implemented:
1. Built in commands (cd, exit, clear [not really built in])
2. Simple commands (ie "ls -l" "grep include shell.c")

Implemented but buggy:
1. Piped commands
	a. For some reason if you enter a piped command first thing after
			running, the piped portion is omitted.
	b. After the first command is entered, piping usually works, but
			sometimes enters an infinite loop of blank commands (^C is needed
			to end the loop)
2. I/O Redirection:
	a. Directing console output to a file is implemented, but such a
			command terminates shell execution for an unknown reason.
	b. Input redirection not implemented.

Not Implemented:
-Commands Running in background
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/wait.h>
#include<sys/types.h>

#define MAX_BUF 256 //max input length
#define MAX_ARG 20 //max number of cmd line args

void runShell();
void printPrompt();
void parseLine(char *input, char** args);
void processInput(char **tokens);
void parsePipedLine(char *input, char** arg1, char** arg2);
void processPipedInput(char **tokens1, char **tokens2);
void parseRedirectedLine(char *input, char** arg1, char** arg2);
void processRedirectedInput(char **tokens1, char **tokens2);
void exitShell();
void execute_simple(char **tokens);
int checkPipe(char *input);
int checkCarrot(char *input);
void execute_cd(char **tokens);

int main(){
	runShell();

	return 0;
}

/*
Basic shell engine. First clears the terminal and displays
a welcome message. Then enters the loop which shows the prompt,
then gets a line of user input, parses the input, and executes
accordingly.
*/
void runShell(){
	char input[MAX_BUF];
	char* arguments[MAX_ARG];
	char* arguments2[MAX_ARG];
	system("clear");
	printf("Welcome to my shell!!\nBy Nate Kell\n\n");

	while(1){
		printPrompt();
		fgets(input, MAX_BUF, stdin);
		if(input != NULL){
			if(checkPipe(input) == 0){
				parsePipedLine(input, arguments, arguments2);
				processPipedInput(arguments, arguments2);
			}else if(checkCarrot(input) == 0){
				parseRedirectedLine(input, arguments, arguments2);
				processRedirectedInput(arguments, arguments2);
			}else{
				parseLine(input, arguments);
				processInput(arguments);
			}
		}
	}
}

/*
Displays the shell prompt, which is the working directory
followed by an arrow.
*/
void printPrompt(){
	char buf[MAX_BUF];

	printf("\n%s> ", getcwd(buf, MAX_BUF));
	fflush(stdin);
	fflush(stdout);
}

/*
Splits a command line into tokens (strings separated by spaces).
*/
void parseLine(char *input, char** args){
	const char d[2] = " ";
	char* currToken = strtok(input, d);
	int counter = 0;

	while(currToken != NULL){
		args[counter] = currToken;
		counter++;
		currToken = strtok(NULL, d);
	}

	args[counter] = NULL;
}

/*
If no pipe or carrot (redirection) is found, execute the built in
or simple command with corresponding arguments.
*/
void processInput(char **tokens){
	if(strcmp(strtok(tokens[0], "\n"), "exit") == 0){
		exitShell();
		return;
	}

	if(strcmp(strtok(tokens[0], "\n"), "clear") == 0){
		system("clear");
		return;
	}

	if(strcmp(tokens[0], "cd") == 0){
		execute_cd(tokens);
		return;
	}

	execute_simple(tokens);
}

/*
Splits a piped command in half and tokenizes each half into arrays.
*/
void parsePipedLine(char *input, char** arg1, char** arg2){
	int counter = 0;
	char* inputs[MAX_ARG];
	const char pipe[2] = "|";
	inputs[counter] = strtok(input, pipe); //string up to pipe
	while(inputs[counter+1] != NULL){
		counter++;
		inputs[counter] = strtok(NULL, pipe);
	}
	parseLine(inputs[0], arg1);
	parseLine(inputs[1], arg2);
}

/*
Connects to input and output streams to pipe two commands together
and executes them accordingly. Buggy but implemented.
*/
void processPipedInput(char **tokens1, char **tokens2){
	int status;

	int pipefd[2];
	pipe(pipefd);

	if (fork() == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], 1);
		execvp(tokens1[0], tokens1);
	} else if (fork() == 0) {
		close(pipefd[1]);
		dup2(pipefd[0], 0);
		execvp(tokens2[0], tokens2);
	}

	close(pipefd[0]);
	close(pipefd[1]);

	wait(0);
	wait(0);
}

/*
Splits input line into a command with arguments and an output filename.
*/
void parseRedirectedLine(char *input, char** arg1, char** arg2){
	int counter = 0;
	char* inputs[MAX_ARG];
	const char carrot[2] = ">";
	inputs[counter] = strtok(input, carrot);
	while(inputs[counter+1] != NULL){
		counter++;
		inputs[counter] = strtok(NULL, carrot);
	}
	parseLine(inputs[0], arg1);
	parseLine(inputs[1], arg2);
}

/*
Create output file and write command output to it.
*/
void processRedirectedInput(char **tokens1, char **tokens2){
	int out;

  out = open(tokens2[0], O_WRONLY | O_TRUNC | O_CREAT);
  dup2(out, 1);

  execvp(tokens1[0], tokens1);
	close(out);
}

/*
Forks a child and executes a simple comand with an array of arguments.
*/
void execute_simple(char** tokens){
	int status;
	pid_t pid;

	char *argv[20];
	argv[0] = strtok(tokens[0], "\n");

	int c = 1;

	while(tokens[c] != NULL){
		argv[c] = strtok(tokens[c], "\n");
		c++;
	}

	argv[c] = NULL;
	pid = fork();
	if (pid == 0){
		if(execvp(argv[0], argv) == -1){ // child: call execv with the path and the args
			printf("invalid command.");
			fflush(stdin);
		}
		exit(-1);
  } else {
		waitpid(pid, &status, WUNTRACED);
	}
}

/*
Handles the built in change directory command.
*/
void execute_cd(char **tokens){
	if(tokens[1] == NULL){ //if no argument given, return
		printf("\n");
		return;
	}
	if(chdir(strtok(tokens[1], "\n")) != 0)
		printf("Directory does not exist.\n");
}

/*
Simple method to determine whether user input contains a pipe.
*/
int checkPipe(char *input){
	int counter = 0;
	char c = input[counter];
	while(c != '\0'){
		c = input[counter];
		if(c == '|') return 0;
		counter++;
	}
	return 1;
}

/*
Simple method to determine whether user input contains a redirection.
*/
int checkCarrot(char *input){
	int counter = 0;
	char c = input[counter];
	while(c != '\0'){
		c = input[counter];
		if(c == '>') return 0;
		counter++;
	}
	return 1;
}

/*
Exits the shell.
*/
void exitShell(){
	printf("\nExiting this shell...\n\n");
	exit(1);
}
