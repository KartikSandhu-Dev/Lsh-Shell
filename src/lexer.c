#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

TokenList tokenize(const char *buffer) {
	Lexer lex = {0};
	lex.source = buffer;
	lex.pos = 0;

	TokenList token_list = {0};
	token_list.capacity = DEF_TOKEN_CAPACITY;
	token_list.count = 0;
	token_list.tokens = malloc(sizeof(Token) * token_list.capacity);

	while(1) {
		Token tok = next_token(&lex);
		token_list.tokens[token_list.count] = tok;

		if(token_list.count >= token_list.capacity) {
			token_list.tokens = realloc(token_list.tokens, 
				sizeof(Token)*(token_list.count + 5));
		}

		if(tok.token_type == TOKEN_EOF) { break; }

		token_list.count++;
	}

	return token_list;
}

static bool not_special_char(char input) {
	char special_char[] = {'|', '>', '<', '\0', '\n'};

	bool not_special = true;
	for(int i = 0; i < (int)sizeof(special_char); i++) {
		if(input == special_char[i]) {
			not_special = false;
			return not_special;
		}
	}
	return not_special;
}

Token next_token(Lexer *lex) {
	skip_whitespace(lex);

	if(not_special_char(current(lex))){
		return read_word(lex);
	}

	Token token = {0};
	switch (current(lex)) {
		case '|':
			token.token_type = TOKN_PIPE;
			break;
		case '>':
			if(peek(lex) == '>') {
				token.token_type = TOKEN_APPEND;
				advance(lex);
			} else {
				token.token_type = TOKEN_REDIR_OUT;
			}
			break;

		case '<':
			token.token_type = TOKEN_REDIR_IN;
			break;

		case '\0':
			token.token_type = TOKEN_EOF;
			break;

		default:
			fprintf(stderr, "Used unexpected token %c", current(lex));
			exit(1);
			break;
	}
	advance(lex);

	return token;
}

void skip_whitespace(Lexer *lex) {
	while(isspace(lex->source[lex->pos])) {
		lex->pos++;
	}
}

char current(Lexer *lex) {
	return lex->source[lex->pos];
}

void advance(Lexer *lex) {
	lex->pos++;
}

char peek(Lexer *lex) {
	return lex->source[lex->pos + 1];
}

Token read_word(Lexer *lex) {
	bool in_quote = false;
	char quote;

	size_t start = lex->pos;

	while(1) {
		char c = current(lex);

		if(c == '\0') { break; }

		if(!in_quote) {
			if(isspace(c)) {
				break;
			}
			if(c == '"' || c == '\'') {
				in_quote = true;
				quote = c;
			}
		} else {
			if(c == quote) {
				in_quote = false;
			}
		}

		advance(lex);
	}

	size_t len = lex->pos - start;

	Token token = {0};
	token.token_type = TOKEN_WORD;
	token.value = malloc(sizeof(char)*(len + 1));

	memcpy(token.value, lex->source + start, len);

	token.value[len] = '\0';

	return token;
}

void clean_tokens(TokenList *token_list) {
	size_t pos = 0;
	while(1) {
		if(token_list->tokens[pos].token_type == TOKEN_WORD) {
			free(token_list->tokens[pos].value);
		}
		
		if(token_list->tokens[pos].token_type == TOKEN_EOF) {
			break;
		}

		pos++;
	}
	free(token_list->tokens);
}

void print_tokens(TokenList *token_list) {
	size_t pos = 0;
	while(1) {
		switch (token_list->tokens[pos].token_type) {
            case TOKEN_WORD:
            	printf("WORD(%s)\n", token_list->tokens[pos].value);
            	break;
            case TOKN_PIPE:
            	printf("PIPE(|)\n");
            	break;
            case TOKEN_REDIR_IN:
            	printf("REDIR_IN(<)\n");
            	break;
            case TOKEN_REDIR_OUT:
            	printf("REDIR_OUT(>)\n");
            	break;
            case TOKEN_APPEND:
            	printf("APPEND(>>)\n");
            	break;
            case TOKEN_EOF:
            	printf("EOF\n");
            	return;
        }
        pos++;
    }
}