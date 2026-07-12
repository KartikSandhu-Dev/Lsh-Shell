#include "exec/builtin.h"
#include "shell/history.h"
#include "shell/signal.h"
#include "shell/variable.h"
#include "shell/shell.h"
#include "var/common.h"

#include <linux/limits.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static BuiltIn builtins[] = {
	{"cd", cd_builtin},
	{"exit", exit_builtin},
	{"pwd", pwd_builtin},
	{"echo", echo_builtin},
	{"export", export_builtin},
	{"unset", unset_builtin},
	{"history", history_builtin},
	{"jobs", jobs_builtin},
	{"fg", fg_builtin},
	{"bg", bg_builtin},
    {NULL, NULL}
};

BuiltIn find_builtin(char *command) {
	int pos = 0;
	while(builtins[pos].name != NULL) {
		if(strcmp(command, builtins[pos].name) == 0) {
			return builtins[pos];
		}
		pos++;
	}

	return builtins[pos];
}

int cd_builtin(ASTNode *node, Shell *shell) {
	(void)shell;

	char *dir = malloc(sizeof(char) * PATH_MAX);
	char *home = get_env_value(shell, "HOME");

	if (node->Command.argc == 1) {
		snprintf(dir, PATH_MAX, "%s", home);
	} else if (strcmp(node->Command.argv[1], "~") == 0) {
		snprintf(dir, PATH_MAX, "%s", home);
	} else if (strncmp(node->Command.argv[1], "~", 1) == 0) {
		snprintf(dir, PATH_MAX, "%s%s", home, node->Command.argv[1] + 1);
	} else {
		snprintf(dir, PATH_MAX, "%s", node->Command.argv[1]);
	}

	if(chdir(dir)) {
		perror("cd");
		return 1;
	}

	set_env_value(shell, "OLDPWD", get_env_value(shell, "PWD"));
	set_env_value(shell, "PWD", dir);
	
	return 0;
}

int exit_builtin(ASTNode *node, Shell *shell) {
	(void)shell;
	int code = 0;

	if(node->Command.argc > 1) {
		code = atoi(node->Command.argv[1]);
	}

	exit(code);
}

int pwd_builtin(ASTNode *node, Shell *shell) {
	(void)shell;
	(void)node;
	char cwd[PATH_MAX];

	if(getcwd(cwd, sizeof(cwd))) {
		write(STDOUT_FILENO, cwd, strlen(cwd));
		write(STDOUT_FILENO, "\n", 1);
		return 0;
	} else {
		return 1;
	}

}

int echo_builtin(ASTNode *node, Shell *shell) {
	(void)shell;
	for(size_t i = 1; i < node->Command.argc; i++) {
		write(STDOUT_FILENO, node->Command.argv[i], strlen(node->Command.argv[i]));
		write(STDOUT_FILENO, " ", 1);
	}
	write(STDOUT_FILENO, "\n", 1);
	return 0;
}

static bool is_assignment(const char *str) {
	char *equal = strchr(str, '='); // if string contains '=' its assignment

	if(!equal) { return false; }

	if(equal == str) { return false; }

	return true;
}

int export_builtin(ASTNode *node, Shell *shell) {
	if(node->Command.argc == 1) { return 1; }

	int pos = 1;
	while(node->Command.argv[pos] != NULL) {
		char *value = get_shell_var(shell, node->Command.argv[pos]);
		if(value) {
			set_env_value(shell, node->Command.argv[pos], value);

		} else if(is_assignment(node->Command.argv[pos])) {
		    char *equal = strchr(node->Command.argv[pos], '=');

		    *equal = '\0';

		    char *name = strdup(node->Command.argv[pos]);
		    char *value = strdup(equal +1);

		    add_shell_var(shell, (ShellVar){ .name = name, .value = value });

		    set_env_value(shell, name, value);

		    *equal = '=';

		} else {
			set_env_value(shell, strdup(node->Command.argv[pos]), " ");
		}

		pos++;
	}

	return 0;
}

int unset_builtin(ASTNode *node, Shell *shell) {
	if(node->Command.argc == 1) { return 1; }

	int pos = 1;
	while(node->Command.argv[pos] != NULL) {
		if(unset_env_value(shell, node->Command.argv[pos]) == 1) { return 1; }
		pos++;
	}

	return 0;
}

int history_builtin(ASTNode *node, Shell *shell) {
	if(node->Command.argc > 1) {
		int index = atoi(node->Command.argv[1]);
		char *cmd = history_get(shell, index);
		write(STDOUT_FILENO, cmd, strlen(cmd));
		write(STDOUT_FILENO, "\n", 1);
	} else {
		history_print(shell);
	}
	return 0;
}

int jobs_builtin(ASTNode *node, Shell *shell) {
	(void)node;

	for(size_t i = 0; i < shell->joblist.count; i++) {
		Job *job = &shell->joblist.jobs[i];

		switch (job->status) {
			case JOB_RUNNING:
				printf("|%d| Running (%s)\n", job->id, job->command);
				break;
			case JOB_STOPPED:
				printf("|%d| Stopped (%s)\n", job->id, job->command);
				break;
			case JOB_DONE:
				printf("|%d| Done (%s)\n", job->id, job->command);
				break;
		}
	}
	return 0;
}

int bg_builtin(ASTNode *node, Shell *shell) {
	int id_user;

	if(node->Command.argv[1] == NULL) { id_user = 1; } 
	else { id_user = atoi(node->Command.argv[1]); }

	int job_idx = find_job_byid(shell, id_user);

	if(job_idx == -1) {
		fprintf(stderr, "Job: id |%s| does not exist\n", node->Command.argv[1]);
		return 1;
	}

	Job *job = &shell->joblist.jobs[job_idx];

	switch (job->status) {
		case JOB_RUNNING:
			printf("Job: id |%d| is already running\n", job->id);
			return 0;
		case JOB_STOPPED:
			printf("Job: id |%d| has been started again\n", job->id);
			break;
		case JOB_DONE:
			printf("Job: id |%d| is already done\n", job->id);
			return 0;
	}

	if(kill(-job->pgid, SIGCONT) < 0) {
		perror("bg: kill (SIGCONT) failed");
		return -1;
	}

	job->status = JOB_RUNNING;

	return 0;
}

int fg_builtin(ASTNode *node, Shell *shell) {
	int id_user;

	if(node->Command.argv[1] == NULL) { id_user = 1; } 
	else { id_user = atoi(node->Command.argv[1]); }

	int job_idx = find_job_byid(shell, id_user);

	if(job_idx == -1) {
		fprintf(stderr, "Job: id %s does not exist\n", node->Command.argv[1]);
		return 1;
	}

	Job *job = &shell->joblist.jobs[job_idx];

	int status;

	tcsetpgrp(STDIN_FILENO, job->pgid);

	if(kill(-job->pgid, SIGCONT) < 0) {
		perror("bg: kill (SIGCONT) failed");
		tcsetpgrp(STDIN_FILENO, getpgrp());
		return -1;
	}

	pid_t ret = waitpid(-job->pgid, &status, WUNTRACED);

	if(ret == -1) {
		perror("waitpid");
		tcsetpgrp(STDIN_FILENO, getpgrp());
		return 1;
	}

	tcsetpgrp(STDIN_FILENO, getpgrp());

	if (WIFSTOPPED(status)) {
		job->status = JOB_STOPPED;
	} else if (WIFEXITED(status) || WIFSIGNALED(status)) {
		clean_job(shell, job_idx);
	}

	return 0;
}