#include "shell/shell.h"
#include "shell/signal.h"
#include "shell/variable.h"
#include "shell/history.h"

#include "var/config.h"
#include "parse/lexer.h"
#include "parse/parser.h"
#include "exec/execute.h"

#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

void shell_init(char **envp) {
	Shell shell; 

	// give shell its values
	shell.envp = duplicate_env(envp);
	history_init(&shell);
	shell_var_init(&shell);
	sig_shell_init(&shell);

	// set env variables for the shell
	char shlvl[16];
	snprintf(shlvl, sizeof(shlvl), "%d", atoi(get_env_value(&shell, "SHLVL")) + 1);

	set_env_value(&shell, "SHELL", SHELL_LOCATION);
	set_env_value(&shell, "SHLVL", shlvl);
	set_env_value(&shell, "OLDPWD", "/");

	// ------MAIN SHELL LOOP--------
	while(1) {
		// print prompt
		print_prompt();
		
		// read line
		int len = read_line(shell.shell_buffer, sizeof(shell.shell_buffer));
		if(len <= 0) { continue; }

		history_add(&shell); // add the line to history

		// tokenise
		TokenList token_list = tokenize(shell.shell_buffer);

		// parse
		ASTNode *ast = parse_tokens(&token_list);

		// execute
		shell.last_status = execute(ast, &shell);

		// add last status of the process to the "?" named shell variable
		add_last_status(&shell);

		// check the status of jobs
		check_jobs(&shell);

		// cleanup
		clean_ASTs(ast);
		clean_tokens(&token_list);
	}
}

void print_prompt() {
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	const char *home = getenv("HOME");

	// "eternal" in red, "@" in white, path in blue, "$" in white
	printf("\033[31m%s\033[37m@", SHELL_NAME);
	if (strncmp(cwd, home, strlen(home)) == 0) {
		printf("\033[34m~%s\033[37m#\033[0m ", cwd + strlen(home));
	} else {
		printf("\033[34m~%s\033[37m#\033[0m ", cwd);
	}

	fflush(stdout);
}

int read_line(char *buffer, const int buffer_len) {
	int len = read(STDIN_FILENO, buffer, buffer_len);

	if(len <= 0) { return -1; }

	if(buffer[len - 1] == '\n') { len--; }

	buffer[len] = '\0';

	return len;
}
