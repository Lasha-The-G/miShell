#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#define SHELLBUF 256
#define ARGNUM 64
#define DELIM " \t"

#define READ_END 0
#define WRITE_END 1

// MUST DO:
// exit handling (done)
// builtin cd    (done)
// background processes and ctrl + z
// 		Fix related ^^ bugs:
// 			ctrl+c after calling sleep gives two ">> "
// 			after running a background process the input lines are mixed up

// MY SHITTY IDEAS
// customizable input line
// parse for configuration (alias)
// add builtins
// add history with arrows

// gag functilnality:
// ctrl+c message
// writing a color changes color
// shell modes (dumbass/hacker/god modes)

typedef struct arginfo{
	char *line;
	char **args;
} Info;
char bg;
char pike;

// Forward declarations (fundamental)
char *get_input();
char **parse_line(char *line);
int exec_prog(char **args);
int start_prog(Info* info);
// Fancy extensions
void print_prompt();

// Handlers
void shutdown_message(int signal){
	write(1, "\nUse \"exit\" builtin function to safely exit.\n", 45);
	write(1, ">> ", 3);
}
void chld_term(int signal){
	int olderrno = errno;
	while (waitpid(-1, NULL, WNOHANG) > 0) {
		//write(1, "Handler reaped child\n", 21);
	}
	//if (errno != ECHILD)
	//	write(1, "waitpid error", 13);
	errno = olderrno;
}

//VVV define all builtins here VVV//
void builtin_cd(Info* info){
	if (chdir(info->args[1]) != 0)
		perror("chdir error");
}

void builtin_exit(Info* info){
	printf("\nEXITING GRACEFULLY\n");
	exit(0);
}

int num_builtins = 2;
char *builtin_names[] = {"exit", "cd"};
void (*builtins[])(Info*) = {builtin_exit, builtin_cd};
// ############################## //


int main(){
	Info* info = (Info*) malloc(sizeof(Info));
	int badCase = 0;
	int i;

	signal(SIGCHLD, chld_term);
	signal(SIGINT, shutdown_message);

	do{
		print_prompt();
		info->line = get_input();
		info->args = parse_line(info->line);
		badCase = start_prog(info);
		
		free(info->args);
		free(info->line);
	} while(!badCase);
}

/// handle builtins ///
int builtin(char *progname){
	int i;
	for (i = 0; i < num_builtins; i++){
		if(strcmp(builtin_names[i], progname) == 0)
			return 1;	
	}
	return 0;
}

void run_builtin(Info* info){
	int i;
	pid_t pid;
	for(i = 0; i< num_builtins; i++){
		if(strcmp(builtin_names[i], info->args[0]) == 0){
			if((pid = fork()) == 0){
				builtins[i](info);
			} else {
				wait(NULL);
				exit(0);
			}
		}
	}
}

int start_prog(Info* info){
	if (builtin(info->args[0])){
		run_builtin(info);
	} else {
		exec_prog(info->args);
	}
	return 0;
}

/// Little nice things ///
void print_prompt(){
	printf(">> ");//in future expand to fancy input lines
}

/// required functionality ///
int handle_pipes(char **args){
	pid_t pid;
	int fd[2];
	int i = 0;
	int status;

	char **args1;
	char **args2;

	args1 = args;
	char *temp;

	for(temp = args[0]; temp != NULL; temp = args[i]){
		if (strcmp(temp, "|") == 0){
			args[i] = NULL;
			args2 = args+i+1;
		}
		i++;
	}

	pipe(fd);
	if( (pid = fork()) == 0 ){
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[WRITE_END]);
        close(fd[READ_END]);
		execvp(args1[0], args1);
	} else {
		waitpid(pid, &status, 0);
		if( (pid = fork()) == 0 ){
            dup2(fd[READ_END], STDIN_FILENO);
            close(fd[READ_END]);
            close(fd[WRITE_END]);
			execvp(args2[0], args2);
		} else {
			close(fd[READ_END]);
			close(fd[WRITE_END]);
			waitpid(pid, &status, 0);
		}
	}
}

char* get_input(){
	int bufsize = SHELLBUF;
	char *buffer = (char*) malloc(bufsize);
	int position = 0;
	int c;

	while(1){
		c = getchar();
		if(c == EOF || c == '\n'){
			buffer[position] = '\0';
			break;
		} else {
			buffer[position] = c;
			position++;
			// in future add expandable shell buffer
		}
	}
	buffer[position] = '\0';
	return buffer;
}

char **parse_line(char *line){
	bg = 0;
	pike = 0;
	char **args = (char**) malloc(ARGNUM * sizeof(char*));
	int pos = 1;

	args[0] = strtok(line, DELIM);
	if(args[0] == NULL)
		return args;  // no program specified	

	while(1){
		args[pos] = strtok(NULL, " \t");
		if(args[pos] == NULL){
			if(strcmp(args[pos-1], "&") == 0){
				bg = 1;
				args[pos-1] = NULL;
			}
			return args;  // strings ended	
		} else {
			if(strcmp(args[pos], "|") == 0)
				pike = 1;
		}
		pos++;
	}
}

int exec_prog(char **args){
	pid_t pid;
	int status;

	if(pike == 1){
		handle_pipes(args);	
		return 0;
	}

	if((pid = fork()) == 0){
		execvp(args[0], args);
		printf("execvp failed, shouldn't be here.\n");
        exit(1);
	}else{
		if(bg == 0)
			waitpid(pid, &status, 0);
	}
	return 0;
}
