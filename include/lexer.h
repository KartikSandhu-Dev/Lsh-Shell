#ifndef LEXER_H
#define LEXER_H

#include <stdlib.h>

#define DEF_TOKEN_CAPACITY 20

typedef struct Lexer {
	const char *source;
	size_t pos;
} Lexer;

typedef enum TokenType {
	TOKEN_WORD, // echo cat ls
	TOKN_PIPE,	// |
	TOKEN_REDIR_IN, // <
	TOKEN_REDIR_OUT, // >
	TOKEN_APPEND, // >>
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

char current(Lexer *lex);
void advance(Lexer *lex);
char peek(Lexer *lex);

void skip_whitespace(Lexer *lex);
Token read_word(Lexer *lex);

void clean_tokens(TokenList *token_list);
void print_tokens(TokenList *token_list);

#endif