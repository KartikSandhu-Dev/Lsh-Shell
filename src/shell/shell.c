#include "shell/shell.h"

#include "var/config.h"
#include "var/common.h"

#include "parse/lexer.h"
#include "parse/parser.h"
#include "shell/history.h"
#include "exec/execute.h"

#include <linux/limits.h>
#include <unistd.h>

void shell_init(char **envp) {
	char shell_buffer[SHELL_BUFFER_LEN];
	history_init();

	// MAIN SHELL LOOP 
	while(1) {
		// print prompt
		print_prompt();
		
		// read line
		int len = read_line(shell_buffer, sizeof(shell_buffer));
		if(len <= 0) { continue; }

		history_add(shell_buffer); // add the line to history

		// tokenise
		TokenList token_list = tokenize(shell_buffer);

		// parse
		ASTNode *ast = parse_tokens(&token_list);

		// execute
		execute(ast, envp);

		// cleanup
		clean_ASTs(ast);
		clean_tokens(&token_list); 
	}
}

void print_prompt() {
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));

	const char *home = getenv("HOME");

	printf("%s@", SHELL_NAME);

	if(strncmp(cwd, home, strlen(home)) == 0) {
		printf("~%s$ ", cwd + strlen(home));
	} else {
		printf("~%s$ ", cwd);
	}

	fflush(stdout);
}

int read_line(char *buffer, const int buffer_len) {
	int len = read(STDIN_FILENO, buffer, buffer_len);

	if(len <= 0) { return -1; }

	if(buffer[len - 1] == '\n') {
		len--;
	}

	buffer[len] = '\0';

	return len;
}
