#include "lexer.h"
#include "var/common.h"
#include "var/config.h"

static char current(Lexer *lex) {
	return lex->source[lex->pos];
}

static void advance(Lexer *lex) {
	lex->pos++;
}

static char peek(Lexer *lex) {
	return lex->source[lex->pos + 1];
}

TokenList *tokenize(const char *buffer) {
	Lexer lex = {0};
	lex.source = buffer;
	lex.pos = 0;

	TokenList *token_list = malloc(sizeof(TokenList));
	token_list->capacity = MAX_ARGS;
	token_list->count = 0;
	token_list->tokens = malloc(sizeof(Token) * token_list->capacity);

	while(1) {
		if(token_list->count >= token_list->capacity) {
			token_list->capacity += 5; 
			token_list->tokens = realloc(token_list->tokens, 
				sizeof(Token)*token_list->capacity);
		}

		Token tok = next_token(&lex);
		token_list->tokens[token_list->count] = tok;

		if(tok.token_type == TOKEN_EOF) { break; }

		token_list->count++;
	}

	return token_list;
}

static bool not_special_char(const char input) {
	char special_char[] = {'|', '>', '<', '&', '\0', '\n', '\t'};

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
			token.token_type = TOKEN_PIPE;
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

		case '&':
			if(peek(lex) == '&') {
				token.token_type = TOKEN_AND;
				advance(lex);
			} else {
				token.token_type = TOKEN_BACKGROUND;
			}
			break;

		case '\0':
			token.token_type = TOKEN_EOF;
			break;

		default:
			fprintf(stderr, "Used unexpected token %c", current(lex));
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

static char *strip_quotes(char *input, size_t len) {
	if(input[0] == '\'' || input[0] == '"') {
		if(len < 2) { return strdup(input); }

		char *out = malloc(len - 1);
		memcpy(out, input+1, len-2);
		out[len-2] = '\0';

		return out;
	}
	return input;
}

Token read_word(Lexer *lex) {
	bool in_quote = false;
	char quote;

	size_t start = lex->pos;

	while(1) {
		char c = current(lex);

		if(c == '\0') { break; }

		if(!in_quote) {
			if(isspace(c) || !not_special_char(c)) {
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

	token.value = strip_quotes(token.value, len);

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
	free(token_list);
}

void print_tokens(TokenList *token_list) {
	size_t pos = 0;
	while(1) {
		switch (token_list->tokens[pos].token_type) {
            case TOKEN_WORD:
            	printf("WORD(%s)\n", token_list->tokens[pos].value);
            	break;
            case TOKEN_PIPE:
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
            case TOKEN_AND:
            	printf("AND(&&)\n");
            	break;
            case TOKEN_BACKGROUND:
            	printf("BACKGROUND(&)\n");
            	break;
            case TOKEN_EOF:
            	printf("EOF\n");
            	return;
              break;
            }
        pos++;
    }
}