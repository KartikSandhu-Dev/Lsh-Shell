#ifndef VARIABLE_H
#define VARIABLE_H

#include "parse/parser.h"
#include "var/common.h"

typedef struct Shell Shell ;

typedef struct ShellVar {
	char *name;
	char *value;
} ShellVar;

typedef struct ShellVarList {
	ShellVar *s_vars;
	size_t count;
	size_t capacity;
} ShellVarList;

void shell_var_init(Shell *shell);
int expand_variables(Shell *shell, ASTNode *node);
char **duplicate_env(char **envp);

char *get_env_value(Shell *shell, const char *name);
int set_env_value(Shell *shell, const char *name, const char *value);
int unset_env_value(Shell *shell, const char *name);

char *get_shell_var(Shell *shell, const char *name);
int add_shell_var(Shell *shell, ShellVar shell_var);
int update_shell_var(Shell *shell, ShellVar shell_var);

void add_last_status(Shell *shell);

#endif