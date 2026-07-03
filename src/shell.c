#include "shell.h"
#include <stdio.h>
#include <string.h>
#include "lexer.h"

#define SHELL_BUFFER_LEN 256
#define SHELL_NAME "eternal>"

void shell_init() {
	char shell_buffer[SHELL_BUFFER_LEN];

	while(1) {
		TokenList token_list = {0};
		
		printf("%s ", SHELL_NAME);
		fgets(shell_buffer, sizeof(shell_buffer), stdin);

		shell_buffer[strcspn(shell_buffer, "\n")] = '\0';

		token_list = tokenize(shell_buffer);

		print_tokens(&token_list);
		clean_tokens(&token_list);
	}
}