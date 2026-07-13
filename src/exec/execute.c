#include "exec/execute.h"
#include "exec/builtin.h"

#include "shell/signal.h"
#include "shell/variable.h"
#include "shell/shell.h"

#include "var/common.h"
#include "parse/parser.h"

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
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
			{
				Pipeline pipeline = {0};
				collect_pipeline(node, &pipeline);
				return execute_pipeline(&pipeline, shell, false);
			}
		case NODE_AND:
			return execute_and(node, shell);
		case NODE_BACKGROUND:
			return execute_background(node, shell);
		default:
			return -1;
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
	(void)shell;
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
				default:
					close(prev_fd[1]);
					close(prev_fd[0]);
					free(prev_fd);
					fprintf(stderr, "Unknown redirection type\n");
					return NULL;
			}

			// open the file in the filepath
			int fd = open(node->Command.redirs[i].file, oflags, 0644);
			if(fd == -1) { 
				perror("open");
				close(prev_fd[1]);
				close(prev_fd[0]);
				free(prev_fd);
				return NULL; 
			}

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

static void exec_external(ASTNode *node, Shell *shell) {
	sig_child_init(shell); // restore the signals to the default

	char *path = resolve_path(shell, node->Command.argv[0]); // find the executable in the path
	if(!path) { exit(EXIT_FAILURE); } 

	// check for redirs, if present execute them
	int *fd = apply_redir(node, shell);
	if (fd == NULL) { exit(EXIT_FAILURE); }

	// if no redir execute normally
	execve(path, node->Command.argv, shell->envp);

	perror("execve");
	exit(EXIT_FAILURE);
}

int execute_command(ASTNode *node, Shell *shell) {
	// handle commands that consist only of redirections
	if (node->Command.argv[0] == NULL) {
		int *fd = apply_redir(node, shell);
		if (!fd) { return 1; }

		reset_redir(fd);
		return 0;
	}

	BuiltIn builtin = find_builtin(node->Command.argv[0]);
	// if the command is builtin, execute it
	if(builtin.name != NULL) {
		// if redir apply redir (makes stdout fd something else)
		int *fd = apply_redir(node, shell);
		if (fd == NULL) { return 1; }

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
		// making the child its own pgid leader
		// if it was background the execute_background command already sets the pgid leader
		if(shell->is_interactive) { setpgid(0, 0); }

		exec_external(node, shell); // execute commands in PATH 
	}

	if(shell->is_interactive) { setpgid(pid, pid); }

	int status = 0;
	pid_t pids[1] = {pid};

	if(shell->is_interactive) {
		tcsetpgrp(STDIN_FILENO, pid); // give the terminal to the child

		waitpid(pid, &status, WUNTRACED); // wait for the child

		// give the terminal back to the shell after child finishes
		tcsetpgrp(STDIN_FILENO, getpgrp());

		if(WIFSTOPPED(status)) {
			add_job(shell, pid, pids, 1);
			shell->joblist.jobs[shell->joblist.count-1].status = JOB_STOPPED;
			return 128 + WSTOPSIG(status);

		} else if(WIFSIGNALED(status)) {
			return 128 + WTERMSIG(status);

		} else if(WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}

	} else {
		int status;
		waitpid(pid, &status, 0);

		if(WIFEXITED(status)) { return WEXITSTATUS(status); }

		return 1;
	}
	return 0;
}

// for executing commands in child process (for pipes, ands)
static void exec_child(ASTNode *node, Shell *shell) {
	BuiltIn builtin = find_builtin(node->Command.argv[0]);

	if(builtin.name != NULL) {
		int *fd = apply_redir(node, shell);
		if (fd == NULL) { return; }

		int status = builtin.func(node, shell);
		reset_redir(fd);
		exit(status); // because its in a child process
	} else {
		exec_external(node, shell);
	}
}

void collect_pipeline(ASTNode *node, Pipeline *pipeline) {
	if(node->ast_type == NODE_PIPE) {
		collect_pipeline(node->Binary.left, pipeline);
		collect_pipeline(node->Binary.right, pipeline);
	} else {
		pipeline->commands[pipeline->count++] = node;
	}
}

int execute_pipeline(Pipeline *pl, Shell *shell, bool background) {
	int pipefds[pl->count - 1][2]; // pipe one less than commands curl | grep -> 1 pipe
	// 0 -> read, 1 -> write

	// make pipes
	for(int i = 0; i < pl->count - 1; i++) {
		if(pipe(pipefds[i]) == -1) {
			perror("pipe");
			return 1;
		}
	}

	pid_t pids[pl->count]; // pids array
	pid_t pgid;

	for(int i = 0; i < pl->count; i++) {
		pids[i] = fork(); // child pid

		if (pids[i] == -1) {
    		perror("fork");
    		return 1;
		}

		if(pids[i] == 0) {
			sig_child_init(shell); // init signals to default on child

			// stdin
			if(i > 0) {
				dup2(pipefds[i-1][0], STDIN_FILENO);
			}
			// stdout
			if(i < pl->count - 1) {
				dup2(pipefds[i][1], STDOUT_FILENO);
			}

			// child dont need pipes
			for(int j = 0; j < pl->count - 1; j++) {
				close(pipefds[j][0]);
				close(pipefds[j][1]);
			}

			exec_child(pl->commands[i], shell);
		}

		// from the parent side as well 
		if(i == 0) {
			pgid = pids[0];
			setpgid(pids[0], pgid);
		} else {
			setpgid(pids[i], pgid);
		}
	}

	// parent dont need pipes anymore
	for(int i = 0; i < pl->count - 1; i++) {
		close(pipefds[i][0]);
		close(pipefds[i][1]);
	}

	int status;

	if(!background) {
		// give terminal ownership to process group
		tcsetpgrp(STDIN_FILENO, pgid);

		// wait for every child
		for(int i = 0; i < pl->count; i++) {
			waitpid(pids[i], &status, WUNTRACED);
		}

		// give terminal ownership to shell
		tcsetpgrp(STDIN_FILENO, getpgrp());
	}

	if(background) {
		add_job(shell, pgid, pids, pl->count);
	}

	if(WIFSTOPPED(status)) {
		if(!background) {
			add_job(shell, pgid, pids, pl->count);
			shell->joblist.jobs[shell->joblist.count-1].status = JOB_STOPPED;
		}

		return 128 + WSTOPSIG(status);

	} else if(WIFSIGNALED(status)) {
		return 128 + WTERMSIG(status);

	} else if(WIFEXITED(status)) {
		return WEXITSTATUS(status);
	}

	return 1;
}

int execute_and(ASTNode *node, Shell *shell) {
	int status = execute(node->Binary.left, shell);
	if(status == 0) {
		return execute(node->Binary.right, shell);
	}

	return -1;
}

int execute_background(ASTNode *node, Shell *shell) {
	if(node->Background.node->ast_type == NODE_COMMAND) {
		pid_t pid = fork();

		if(pid < 0) {
			perror("fork");
			return 1;
		}

		if(pid == 0) {
			setpgid(0, 0);
			sig_child_init(shell); // init signals to default on child
			exec_child(node->Background.node, shell);
		}

		setpgid(pid, pid);
		pid_t pids[] = {pid};

		add_job(shell, pid, pids, 1);
		shell->joblist.jobs[shell->joblist.count-1].status = JOB_RUNNING;

		return 0;
	} else if(node->Background.node->ast_type == NODE_PIPE) {
		Pipeline pl = {0};
		collect_pipeline(node->Background.node, &pl);
		return execute_pipeline(&pl, shell, true);
	} else {
		pid_t pid = fork();

		if(pid < 0) {
			perror("fork");
			return 1;
		}

		if(pid == 0) {
			setpgid(0, 0);
			sig_child_init(shell); // init signals to default on child
			shell->is_interactive = false;

			int status = execute(node->Background.node, shell);
			exit(status);
		}

		setpgid(pid, pid);
		pid_t pids[] = {pid};

		add_job(shell, pid, pids, 1);
		shell->joblist.jobs[shell->joblist.count-1].status = JOB_RUNNING;
		
		return 0;
	}

	return -1;
}