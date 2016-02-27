#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>
#include "execute.h"
#include "command.h"

static int run_program(Command *cmd);
static int execute_command_with_pipe(Command *cmd);

/** Builtin command handlers */
static int execute_builtin_cd(char *argv[]);
static int execute_builtin_exit(char *argv[]);
/**
* Execute a Command (with or without pipes.)
*/
int execute_command(Command *cmd)
{

	// Check for builtin commands
	if (strcmp(cmd->argv[0], "cd") == 0) {
		return execute_builtin_cd(cmd->argv);
	}

	if (strcmp(cmd->argv[0], "exit") == 0) {
		return execute_builtin_exit(cmd->argv);
	}

	if (cmd->pipe_to == NULL) {
		pid_t child = fork();
		//check for failure
		if (child < 0) {
			perror("fork");
			return EXIT_FAILURE;
		}
		if (child == 0) {
			// Fork return 0, meaning we are the child process
			return run_program(cmd);
		}else{
			//wait for child to terminate
			int status;
			wait(&status);
		}
		return EXIT_SUCCESS;
	}
	else {
		return execute_command_with_pipe(cmd);
	}

	return EXIT_FAILURE;
}

/**
* `cd` -- change directory (builtin command)
*
* Changes directory to a path specified in the `argv` array.
*
* Example:
*      argv[0] = "cd"
*      argv[1] = "csc209/a3"
*
* Your command MUST handle either paths relative to the current working
* directory, and absolute paths relative to root (/).
*
* Example:
*      relative path:      cd csc209/a3
*      absolute path:      cd /u/csc209h/summer/
*
* Be careful and do not assume that any argument was necessarily passed at all
* (in which case `argv[1]` would be NULL, because `argv` is a NULL terminated
* array)
*/
static int execute_builtin_cd(char *argv[])
{
	char *arg = argv[1];
	int is_absolute = (arg && arg[0] == '/');

	if(arg){
		if(is_absolute){
			return chdir(argv[1]);
		}else{
			char cwd[PATH_MAX];
			strcpy(cwd, getcwd(cwd, sizeof (cwd)));
			strcat(cwd, "/");
			strcat(cwd, argv[1]);
			return chdir(cwd);
		}
	}else{
		perror("cd");
		exit(EXIT_FAILURE);
	}
}

/**
* `exit` -- exit the shell (builtin command)
*
* Terminate the shell process with status 0 using `exit(3)`. This function
* should never return.
*
* You can optionally take an integer argument (a status) and exit with that
* code.
*/
static int execute_builtin_exit(char *argv[])
{
	if(argv[1]){
		int status = atoi(argv[1]);
		exit(status);
	}
	exit(0);
}

/**
* Execute the program specified by `cmd->argv`.
*
* Setup any requested redirections for STDOUT, STDIN and STDERR, and then use
* `execvp` to execute the program.
*
* This function should return ONLY IF there is an failure while exec'ing the
* program. This implies that this function should only be run in a forked
* child process.
*/
static int run_program(Command *cmd)
{
	
	if(cmd->in_filename){
		int fd = open(cmd->in_filename, O_RDONLY, 0644);
		if (fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDIN_FILENO) < 0) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		if(close(fd) < 0){
			perror("close");
			exit(EXIT_FAILURE);
		}
	}
	if(cmd->out_filename){
		int fd = open(cmd->out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDOUT_FILENO) < 0) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		if(close(fd) < 0){
			perror("close");
			exit(EXIT_FAILURE);
		}
	}
	if(cmd->err_filename){
		int fd = open(cmd->err_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDERR_FILENO) < 0) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		if(close(fd) < 0){
			perror("close");
			exit(EXIT_FAILURE);
		}
	}
	if (execvp(cmd->argv[0], cmd->argv)) {
		//show command that failed
		fprintf(stderr, "%s: No such file or directory\n", cmd->argv[0]);
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}

/**
* Execute (at least) two Commands connected by a pipe
*/
static int execute_command_with_pipe(Command *cmd)
{
	assert (cmd->pipe_to != NULL);

	int pipefds[2];
	if (pipe(pipefds) < 0) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	pid_t child = fork();
	//check for failure
	if (child < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (child == 0) {
		//child 1
		// Fork return 0, meaning we are the child process
		if(close(pipefds[0]) < 0){
			perror("close");
			exit(EXIT_FAILURE);
		}
		if(close(STDOUT_FILENO) < 0){
			perror("close");
			exit(EXIT_FAILURE);
		}
		// Make the stdout FD actually be the write end of the pipe
		if (dup2(pipefds[1], STDOUT_FILENO) < 0) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		if(run_program(cmd)){
			exit(EXIT_FAILURE);
		}else {
			exit(EXIT_SUCCESS);
		}
	}else{
		//parent 1
		pid_t child2 = fork();
		//check for failure
		if (child2 < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		if (child2 == 0) {
			// Fork return 0, meaning we are the child process
			// Child => Parent
			if(close(pipefds[1]) < 0){
				perror("close");
				exit(EXIT_FAILURE);
			}
			if(close(STDIN_FILENO) < 0){
				perror("close");
				exit(EXIT_FAILURE);
			}
			// Make the stdin FD actually be the read end of the pipe
			if (dup2(pipefds[0], STDIN_FILENO) < 0) {
				perror("dup2");
				exit(EXIT_FAILURE);
			}
			//try execute next command
			if(execute_command(cmd->pipe_to)){
				exit(EXIT_FAILURE);
			}else{
				exit(EXIT_SUCCESS);
			}
		}else{
			//parent 2
			if(close(pipefds[0]) < 0){
				perror("close");
				exit(EXIT_FAILURE);
			}
			if(close(pipefds[1]) < 0){
				perror("close");
				exit(EXIT_FAILURE);
			}
			int num_exited = 0;
			int status;
			while (num_exited < 2) {
				pid_t w;
				// Wait for any child
				w = waitpid(-1, &status, WUNTRACED | WCONTINUED);

				if (w < 0) {
					perror("waitpid");
					exit(EXIT_FAILURE);
				}
				if (WIFEXITED(status)) {
					num_exited++;
				}
				else if (WIFSIGNALED(status)) {
					int termsig = WTERMSIG(status);
				}
				else if (WIFSTOPPED(status)) {
					int stopsig = WSTOPSIG(status);
				}
				else if (WIFCONTINUED(status)) {
				}
			}
		}
	}
	return EXIT_SUCCESS;
}