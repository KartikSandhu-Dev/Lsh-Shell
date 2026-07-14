#include "shell/variable.h"
#include "shell/shell.h"

#include "var/colors.h"
#include "var/config.h"
#include "var/common.h"

void shell_var_init(Shell *shell) {
	shell->vars.capacity = SHELL_VAR_CAPACITY;
	shell->vars.count = 0;
	shell->vars.s_vars = calloc(shell->vars.capacity, sizeof(ShellVar));

	if (shell->vars.s_vars == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
}

int expand_variables(Shell *shell, ASTNode *node) {
	size_t pos = 0;
	while(node->Command.argv[pos] != NULL) {
		if(node->Command.argv[pos][0] == SHELL_VAR_SYMBOL) {
			char *name = strdup(node->Command.argv[pos] + 1);
			char *var = get_env_value(shell, name);

			if (!var) {
			    var = get_shell_var(shell, name);
			}

			free(node->Command.argv[pos]);

			if (var) {
			    node->Command.argv[pos] = strdup(var);
			} else {
			    node->Command.argv[pos] = strdup("");
			    fprintf(stderr, BR_RED "%s:" RESET " unkown variable\n", name);
			    free(name);
			    return 1;
			}
			free(name);
		}
		pos++;
	}
	return 0;
}

char **duplicate_env(char **envp) {
	int count = 0;
	int pos = 0;

	// count variables
	while (envp[count]) { count++; }

	// allocate array (+1 for NULL)
	char **new_env = malloc(sizeof(char *) * (count + 1));
	if (!new_env) { return NULL; }

	// copy each string
	while (pos < count) {
		new_env[pos] = strdup(envp[pos]);
		if (!new_env[pos]) {
			// free already copied strings if malloc fails
			while (pos > 0)
			{
			    pos--;
			    free(new_env[pos]);
			}
			free(new_env);
			return NULL;
		}
		pos++;
	}

	// NULL terminate
	new_env[count] = NULL;

	return new_env;
}

char *get_env_value(Shell *shell, const char *name) {
	int len = strlen(name);
	int pos = 0;

	while(shell->envp[pos]) {
		if(strncmp(shell->envp[pos], name, len) == 0 && shell->envp[pos][len] == '=') {
			return (shell->envp[pos] + len + 1);
		}
		pos++;
	}

	return NULL;
}

int set_env_value(Shell *shell, const char *name, const char *value) {
	int len = strlen(name);
	int pos = 0;
	int count = 0;

	int new_len = strlen(name) + strlen(value) + 2;
	char *new_var = malloc(sizeof(char) * new_len);

	if(!new_var) {
		perror("new_var malloc");
		return -1;
	}

	snprintf(new_var, new_len, "%s=%s", name, value);

	while(shell->envp[pos]) {
		if(strncmp(shell->envp[pos], name, len) == 0 && shell->envp[pos][len] == '=') {
			free(shell->envp[pos]);
			shell->envp[pos] = new_var;

			return 0;
		}

		pos++;
	}

	count = pos;

	char **tmp = realloc(shell->envp, sizeof(char *) * (count + 2));

	if (!tmp) {
		free(new_var);
		perror("shell_envp malloc");
		return -1;
	}

	shell->envp = tmp;

	shell->envp[count] = new_var;
	shell->envp[count+1] = NULL;

	return 0;
}

int unset_env_value(Shell *shell, const char *name) {
	char *value = get_shell_var(shell, name);
	if(value) {
		set_env_value(shell, strdup(name), " ");
		return 0;
	}

	value = get_env_value(shell, name);
	if(value) {
		set_env_value(shell, strdup(name), " ");
		return 0;
	}
	
	fprintf(stderr, BR_RED "%s:" RESET " Variable does not exist\n", name);
	
	return 1;
}

char *get_shell_var(Shell *shell, const char *name) {
	int pos = 0;
	while(shell->vars.s_vars[pos].name != NULL) {
		if(strcmp(shell->vars.s_vars[pos].name, name) == 0) {
			return shell->vars.s_vars[pos].value;
		}
		pos++;
	}
	return NULL;
}

static char *strip_quotes(char *input, size_t len) {
	if(input[0] == '"' || input[0] == '\'') {
		if(len < 2) { return strdup(input); }

		char *out = malloc(len - 1);
		memcpy(out, input+1, len-2);
		out[len-2] = '\0';

		free(input);

		return out;
	}
	return input;
}

int add_shell_var(Shell *shell, ShellVar shell_var) {
	int pos = 0;
	while(shell->vars.s_vars[pos].name != NULL) {
		if(strcmp(shell->vars.s_vars[pos].name, shell_var.name) == 0) {
			return update_shell_var(shell, shell_var);
		}
		pos++;
	}

	if(shell->vars.count >= shell->vars.capacity) {
		shell->vars.capacity += 20;
		ShellVar *tmp = realloc(shell->vars.s_vars, 
								sizeof(ShellVar) * shell->vars.capacity);

		if(!tmp) {
			perror("shell_vars.s_vars");
			return -1;
		}

		shell->vars.s_vars = tmp;
	}

	char *value = strip_quotes(shell_var.value, strlen(shell_var.value));

	shell->vars.s_vars[shell->vars.count].name = shell_var.name;
	shell->vars.s_vars[shell->vars.count].value = value;

	shell->vars.count++;
	shell->vars.s_vars[shell->vars.count].name = NULL;

	return 0;
}

int update_shell_var(Shell *shell, ShellVar shell_var) {
	int pos = 0;
	while(shell->vars.s_vars[pos].name != NULL) {
		if(strcmp(shell->vars.s_vars[pos].name, shell_var.name) == 0) {
			shell->vars.s_vars[pos] = shell_var;
		}
		pos++;
	}

	return 0;
}

void add_last_status(Shell *shell) {
	char buffer[16];
	snprintf(buffer, sizeof(buffer), "%d", shell->last_status);

	ShellVar var = { .name = strdup("?"), .value = strdup(buffer) };

	add_shell_var(shell, var);
}
