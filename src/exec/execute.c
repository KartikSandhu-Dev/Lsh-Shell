#include "exec/execute.h"
#include "exec/builtin.h"
#include "parse/parser.h"

#include "var/common.h"
#include "shell/variable.h"
#include "shell/variable.h"
#include "shell/shell.h"

#include <linux/limits.h>
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

static void execute_redir(ASTNode *node, char *path, Shell *shell) {
	if(node->Command.redir_count > 0) {
		for(size_t i = 0; i < node->Command.redir_count; i++) {
			int oflags;

			if(node->Command.redirs[i].type == REDIR_APPEND ) {
				// if ">>" append to the filepath or create if not present
				oflags = O_WRONLY | O_APPEND | O_CREAT;
			} else {
				// else if ">" "<" truncate and write to the file
				oflags = O_WRONLY | O_CREAT | O_TRUNC;
			}

			// open the file in the filepath
			int fd = open(node->Command.redirs[i].file, oflags, 0644);
			if(fd == -1) { perror("open"); exit(EXIT_FAILURE); }

			dup2(fd, STDOUT_FILENO);
			close(fd);

			execve(path, node->Command.argv, shell->envp);
		}
	}
}

static bool is_assignment(const char *str) {
	char *equal = strchr(str, '=');

	if(!equal) { return false; }

	if(equal == str) { return false; }

	return true;
}

static bool execute_assingment(ASTNode *node, Shell *shell) {
	char *equal = strchr(node->Command.argv[0], '=');
	*equal = '\0';

	ShellVar var = {0};
	var.name = node->Command.argv[0];
	var.value = equal + 1;
	var.exported = false;

	add_shell_var(shell, var);

	*equal = '=';
	return 0;
}

int execute_command(ASTNode *node, Shell *shell) {
	BuiltIn builtin = find_builtin(node->Command.argv[0]);
	// if the command is builtin, execute it
	if(builtin.name != NULL) {
		return builtin.func(node, shell);
	}

	if(is_assignment(node->Command.argv[0])) {
		return execute_assingment(node, shell);
	}

	pid_t pid = fork(); // make child process

	if(pid == 0) {
		char *path = resolve_path(shell, node->Command.argv[0]); // find the executable in the path
		if(!path) { exit(EXIT_FAILURE); } 

		// check for redirs, if present execute them
		execute_redir(node, path, shell);

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
	int fd[2];
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