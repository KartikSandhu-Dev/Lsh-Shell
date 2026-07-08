#include "exec/builtin.h"
#include "shell/history.h"
#include "var/common.h"

#include <linux/limits.h>
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
	const char *dir;

	if(node->Command.argc == 1) {
		dir = getenv("HOME");
	} else {
		dir = node->Command.argv[1];
	}

	if(chdir(dir)) {
		perror("cd");
		return 1;
	}
	
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
		write(STDOUT_FILENO, node->Command.argv[i], strlen(node->Command.argv[1]));
		write(STDOUT_FILENO, " ", 1);
	}
	write(STDOUT_FILENO, "\n", 1);
	return 0;
}

int export_builtin(ASTNode *node, Shell *shell) {
	return 1;
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