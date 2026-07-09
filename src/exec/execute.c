#include "exec/execute.h"
#include "exec/builtin.h"
#include "parse/parser.h"

#include "var/common.h"
#include "shell/variable.h"
#include "shell/variable.h"
#include "shell/shell.h"

#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int execute(ASTNode *node, Shell *shell) {
	if(!node) { return 1;}
	// execution recursive loop

	switch (node->ast_type) {
		case NODE_COMMAND:
			if(expand_variables(shell, node) == 1) { return 1; };
			return execute_command(node, shell);
		case NODE_PIPE:
			return execute_pipe(node, shell);
		case NODE_AND:
			return execute_and(node, shell);
		default:
			return 1;
	}
}

char *resolve_path(Shell *shell, const char *command_name) {
	// if "./" or "../" or "/some/etc" and executable return the filepath
	if(strncmp(command_name, "./", 2) == 0 ||
		strncmp(command_name, "../", 3) == 0 ||
		strncmp(command_name, "/", 1) == 0) {

		if(access(command_name, X_OK) == 0) {
			return strdup(command_name);
		} else {
			fprintf(stderr, "Error: could not execute '%s': %s\n", command_name, strerror(errno));
			return NULL;
		}
	}

	// if not starts with either of those and like "ls" search in path
	const char *PATH = get_env_value(shell, "PATH");
	char *path = strdup(PATH);

	char *dir = strtok(path, ":");

	while(dir != NULL) {
		char candidate[PATH_MAX];

		// path + command_name
		snprintf(candidate, sizeof(candidate), 
			"%s/%s", dir, command_name);

		// if executable return
		if(access(candidate, X_OK) == 0) {
			free(path);
			return strdup(candidate);
		}

		dir = strtok(NULL, ":");
	}

	free(path);	
	fprintf(stderr, "%s: command not found\n", command_name);

	return NULL;
}

static int *apply_redir(ASTNode *node, Shell *shell) {
	int *prev_fd = malloc(sizeof(int) *2);

	prev_fd[1] = dup(STDOUT_FILENO);
	prev_fd[0] = dup(STDIN_FILENO);

	if(node->Command.redir_count > 0) {
		for(size_t i = 0; i < node->Command.redir_count; i++) {
			int oflags;
			int fd_change;

			switch (node->Command.redirs[i].type) {
				case REDIR_IN:
					// if "<" read only
					oflags = O_RDONLY;
					fd_change = STDIN_FILENO;
					break;
				case REDIR_OUT:
					// if if ">" truncate and write to the file
					oflags = O_WRONLY | O_CREAT | O_TRUNC;
					fd_change = STDOUT_FILENO;
					break;
				case REDIR_APPEND:
					// if ">>" append to the file or create if not present
					oflags = O_WRONLY | O_APPEND | O_CREAT;
					fd_change = STDOUT_FILENO;
					break;
			}

			// open the file in the filepath
			int fd = open(node->Command.redirs[i].file, oflags, 0644);
			if(fd == -1) { perror("open"); return NULL; }

			dup2(fd, fd_change);
			close(fd);
		}
	}

	return prev_fd;
}

static void reset_redir(int *fd) {
	dup2(fd[0], STDIN_FILENO);
	dup2(fd[1], STDOUT_FILENO);

	close(fd[0]);
	close(fd[1]);
	free(fd);
}

static bool is_assignment(const char *str) {
	char *equal = strchr(str, '='); // if string contains '=' its assignment

	if(!equal) { return false; }

	if(equal == str) { return false; }

	return true;
}

static int add_assingment(ASTNode *node, Shell *shell) {
	int pos = 0;
	while(node->Command.argv[pos]) {
		if(!is_assignment(node->Command.argv[pos])) {
			fprintf(stderr, "Used something other than assingment in command\n");
			return 1;
		}

		char *equal = strchr(node->Command.argv[pos], '=');
		*equal = '\0';

		ShellVar var = {0};
		var.name = strdup(node->Command.argv[pos]);
		var.value = strdup(equal + 1);

		add_shell_var(shell, var);

		*equal = '=';
		pos++;
	}

	return 0;
}

int execute_command(ASTNode *node, Shell *shell) {
	BuiltIn builtin = find_builtin(node->Command.argv[0]);
	// if the command is builtin, execute it
	if(builtin.name != NULL) {
		// if redir apply redir (makes stdout fd something else)
		int *fd = apply_redir(node, shell);
		int status = builtin.func(node, shell);

		reset_redir(fd); // reset stdout fd after executing func
		return status;
	}

	// if the command is an assignment, execute it
	if(is_assignment(node->Command.argv[0])) {
		return add_assingment(node, shell);
	}

	// if none of the above find in PATH

	pid_t pid = fork(); // make child process

	if(pid == 0) {
		char *path = resolve_path(shell, node->Command.argv[0]); // find the executable in the path
		if(!path) { exit(EXIT_FAILURE); } 

		// check for redirs, if present execute them
		if(!apply_redir(node, shell)) { exit(EXIT_FAILURE); };

		// if no redir execute normally
		execve(path, node->Command.argv, shell->envp);

		perror("execve");
		exit(EXIT_FAILURE);
	}

	int status;
	waitpid(pid, &status, 0); // wait for the child

	return WEXITSTATUS(status);
}

int execute_pipe(ASTNode *node, Shell *shell) {
	int fd[2]; // fd[0] -> reading end, fd[1] -> writing end
	if (pipe(fd) == -1) {
	    perror("pipe");
	    return 1;
	}

	pid_t left = fork();
	if(left == 0) {
		dup2(fd[1], STDOUT_FILENO);

		close(fd[0]);
		close(fd[1]);

		// child process inside another child process
		int exec1 = execute(node->Binary.left, shell);
		exit(exec1); // thats why need to exit
	}

	pid_t right = fork();
	if(right == 0) {
		dup2(fd[0], STDIN_FILENO);

		close(fd[0]);
		close(fd[1]);

		int exec2 = execute(node->Binary.right, shell);
		exit(exec2);
	}

	close(fd[1]);
	close(fd[0]);

	int left_status, right_status;

	// wait for both child processes to end
	waitpid(left, &left_status, 0);
	waitpid(right, &right_status, 0);

	return WEXITSTATUS(right_status);

}

int execute_and(ASTNode *node, Shell *shell) {
	int status = execute(node->Binary.left, shell);

	if(status == 0) {
		return execute(node->Binary.right, shell);
	}

	return 1;
}