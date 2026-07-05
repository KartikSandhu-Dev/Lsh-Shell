#include "execute.h"
#include "parser.h"
#include "var/common.h"

#include <linux/limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int execute(ASTNode *node, char **envp) {
	if(!node) { return 1;}

	switch (node->ast_type) {
		case NODE_COMMAND:
			return execute_command(node, envp);
		case NODE_PIPE:
			return execute_pipe(node, envp);
		case NODE_AND:
			return execute_and(node, envp);
		default:
			return 1;
	}
}

char *resolve_path(const char *command_name) {
	if(strncmp(command_name, "./", 2) == 0 ||
		strncmp(command_name, "../", 3) == 0 ||
		strncmp(command_name, "/", 1) == 0) {

		if(access(command_name, X_OK) == 0) {
			return strdup(command_name);
		} else {
			fprintf(stderr, "Error: could not access '%s': %s\n", command_name, strerror(errno));
			return NULL;
		}
	}

	const char *PATH = getenv("PATH");
	char *path = strdup(PATH);

	char *dir = strtok(path, ":");

	while(dir != NULL) {
		char candidate[PATH_MAX];

		snprintf(candidate, sizeof(candidate), 
			"%s/%s", dir, command_name);

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

static void execute_redir(ASTNode *node, char *path, char **envp) {
	if(node->Command.redir_count > 0) {
		for(size_t i = 0; i < node->Command.redir_count; i++) {
			int oflags;

			if(node->Command.redirs[i].type == REDIR_APPEND ) {
				oflags = O_WRONLY | O_APPEND | O_CREAT;
			} else {
				oflags = O_WRONLY | O_CREAT | O_TRUNC;
			}

			int fd = open(node->Command.redirs[i].file, oflags, 0644);
			if(fd == -1) { perror("open"); exit(EXIT_FAILURE); }

			dup2(fd, STDOUT_FILENO);
			close(fd);

			execve(path, node->Command.argv, envp);
		}
	}
}

int execute_command(ASTNode *node, char **envp) {
	pid_t pid = fork();

	if(pid == 0) {
		char *path = resolve_path(node->Command.argv[0]);
		if(!path) { exit(EXIT_FAILURE); }

		execute_redir(node, path, envp);

		execve(path, node->Command.argv, envp);

		perror("execve");
		exit(EXIT_FAILURE);
	}

	int status;
	waitpid(pid, &status, 0);

	return WEXITSTATUS(status);
}

int execute_pipe(ASTNode *node, char **envp) {
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

		int exec1 = execute(node->Binary.left, envp);
		exit(exec1);
	}

	pid_t right = fork();
	if(right == 0) {
		dup2(fd[0], STDIN_FILENO);

		close(fd[0]);
		close(fd[1]);

		int exec2 = execute(node->Binary.right, envp);
		exit(exec2);
	}

	close(fd[1]);
	close(fd[0]);

	int left_status, right_status;

	waitpid(left, &left_status, 0);
	waitpid(right, &right_status, 0);

	return WEXITSTATUS(right_status);

}

int execute_and(ASTNode *node, char **envp) {
	int status = execute(node->Binary.left, envp);

	if(status == 0) {
		return execute(node->Binary.right, envp);
	}

	return 1;
}