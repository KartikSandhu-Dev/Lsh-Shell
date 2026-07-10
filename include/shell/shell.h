#ifndef SHELL_H
#define SHELL_H

#include "shell/signal.h"
#include "var/config.h"
#include "shell/history.h"
#include "shell/variable.h"

typedef struct Shell {
	char shell_buffer[SHELL_BUFFER_LEN];
	char **envp;
	History history;
	ShellVarList vars;
	JobList joblist;
	int last_status;
} Shell;

void shell_init(char **envp);
void print_prompt();
int read_line(char *buffer, const int buffer_len);

#endif