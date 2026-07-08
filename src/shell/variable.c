#include "shell/variable.h"
#include "var/config.h"
#include "var/common.h"
#include "shell/shell.h"

void shell_var_init(Shell *shell) {
	shell->vars.capacity = SHELL_VAR_CAPACITY;
	shell->vars.count = 0;
	shell->vars.s_vars = malloc(sizeof(ShellVar) * shell->vars.capacity);
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
			    fprintf(stderr, "Unknown variable: %s does not exist\n", name);
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
	while (pos < count)
	{
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

	shell->envp = realloc(shell->envp, sizeof(char *) * (count + 2));

	if (!shell->envp) {
		free(new_var);
		perror("shell_envp malloc");
		return (-1);
	}

	shell->envp[count] = new_var;
	shell->envp[count+1] = NULL;

	return 0;
}

int unset_env_value(Shell *shell, const char *name) {
	return 0;
}

char *get_shell_var(Shell *shell, char *name) {
	int pos = 0;
	while(shell->vars.s_vars[pos].name != NULL) {
		if(strcmp(shell->vars.s_vars[pos].name, name) == 0) {
			return shell->vars.s_vars[pos].value;
		}
		pos++;
	}
	return NULL;
}

int add_shell_var(Shell *shell, ShellVar shell_var) {
	int pos = 0;
	while(shell->vars.s_vars[pos].name != NULL) {
		if(strcmp(shell->vars.s_vars[pos].name, shell_var.name) == 0) {
			update_shell_var(shell, shell_var);
		}
		pos++;
	}

	if(shell->vars.count >= shell->vars.capacity) {
		shell->vars.capacity += 20;
		shell->vars.s_vars = realloc(shell->vars.s_vars, 
								sizeof(ShellVar) * shell->vars.capacity);

		if(!shell->vars.s_vars) {
			perror("shell_vars.s_vars");
			return 1;
		}
	}

	shell->vars.s_vars[shell->vars.count].name = strdup(shell_var.name);
	shell->vars.s_vars[shell->vars.count].value = strdup(shell_var.value);
	shell->vars.s_vars[shell->vars.count].exported = shell_var.exported;

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

