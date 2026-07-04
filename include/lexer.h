#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>

typedef struct Lexer {
	const char *source;
	size_t pos;
} Lexer;

typedef enum TokenType {
	TOKEN_WORD, // echo cat ls
	TOKEN_PIPE,	// |
	TOKEN_REDIR_IN, // <
	TOKEN_REDIR_OUT, // >
	TOKEN_APPEND, // >>
	TOKEN_AND, // &&
	TOKEN_BACKGROUND, // &
	TOKEN_EOF 
} TokenType;

typedef struct Token {
	TokenType token_type;
	char *value; // gonna use for only word token
} Token;

typedef struct TokenList {
	Token *tokens;
	size_t count;
	size_t capacity;
} TokenList;

TokenList tokenize(const char *buffer);

Token next_token(Lexer *lex);

void skip_whitespace(Lexer *lex);
Token read_word(Lexer *lex);

void clean_tokens(TokenList *token_list);
void print_tokens(TokenList *token_list);

#endif