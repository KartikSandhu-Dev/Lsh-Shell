#include "exec/builtin.h"
#include "shell/history.h"
#include "shell/variable.h"
#include "var/common.h"

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static BuiltIn builtins[] = {
	{"cd", cd_builtin},
	{"exit", exit_builtin},
	{"pwd", pwd_builtin},
	{"echo", echo_builtin},
	{"export", export_builtin},
	{"unset", unset_builtin},
	{"history", history_builtin},
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
	char *dir = malloc(sizeof(char) * PATH_MAX);
	char *home = get_env_value(shell, "HOME");

	if(node->Command.argc == 1) {
		dir = home;
	} else {
		// dir = node->Command.argv[1];
		if(strcmp(node->Command.argv[1], "~") == 0) {
			dir = home;
		} else if (strncmp(node->Command.argv[1], "~", 1) == 0) {
			snprintf(dir, PATH_MAX, "%s%s", home, node->Command.argv[1] + 1);
		}
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
	int code = 0;

	if(node->Command.argc > 1) {
		code = atoi(node->Command.argv[1]);
	}

	exit(code);
}

int pwd_builtin(ASTNode *node, Shell *shell) {
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
	for(int i = 1; i < node->Command.argc; i++) {
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

static bool add_assingment(ASTNode *node, Shell *shell, int pos) {
	char *equal = strchr(node->Command.argv[pos], '=');
	*equal = '\0';

	ShellVar var = {0};
	var.name = strdup(node->Command.argv[pos]);
	var.value = strdup(equal + 1);

	add_shell_var(shell, var);

	*equal = '=';

	return 0;
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

	return 1;
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