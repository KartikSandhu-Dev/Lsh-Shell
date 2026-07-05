#include "shell.h"
#include "var/config.h"
#include "var/common.h"

#include "lexer.h"
#include "parser.h"
#include "execute.h"

#include <unistd.h>
#include <sys/wait.h>
#include <linux/limits.h>

void shell_init(char **envp) {
	char shell_buffer[SHELL_BUFFER_LEN];

	while(1) {
		print_prompt();

		char *line = read_line(shell_buffer, sizeof(shell_buffer));
		if(line[0] == '\0' || !line) { continue; }

		TokenList *token_list = tokenize(shell_buffer);

		ASTNode *cmd = parse_tokens(token_list);

		execute(cmd, envp);

		clean_ASTs(cmd);
		clean_tokens(token_list); 
	}
}

void print_prompt() {
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));

	const char *home = getenv("HOME");

	printf("%s@", SHELL_NAME);

	if(strncmp(cwd, home, strlen(home)) == 0) {
		printf("~%s# ", cwd + strlen(home));
	} else {
		printf("~%s# ", cwd);
	}

	fflush(stdout);
}

char *read_line(char *buffer, const int buffer_len) {
	char *input = fgets(buffer, buffer_len, stdin);

	if (!input) { return NULL; }

	buffer[strcspn(buffer, "\n")] = '\0';

	return buffer;
}