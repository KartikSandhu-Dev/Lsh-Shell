#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdlib.h>

typedef struct ASTNode ASTNode;

typedef struct Parser {
	TokenList token_list;
	size_t pos;
} Parser;

typedef enum ASTtype {
	NODE_COMMAND,
	NODE_PIPE,
	NODE_AND,
} ASTtype;

typedef enum RedirType {
	REDIR_IN, // <
	REDIR_OUT, // >
	REDIR_APPEND, // >>
} RedirType;

typedef struct {
    RedirType type;
    char *file;
} Redir;

typedef struct ASTNode {
	ASTtype ast_type;

	union {
		struct {
			size_t argc;
			char **argv;

			Redir *redirs;
			size_t redir_count;
		} Command;

		struct {
			ASTNode *left;
			ASTNode *right;
		} Binary;
	};

} ASTNode;

ASTNode *parse_tokens(TokenList *token_list);

ASTNode *parse_command(Parser *parser);
ASTNode *parse_and(Parser *parser);
ASTNode *parse_pipe(Parser *parser);

void clean_ASTs(ASTNode *node);
void print_ASTs(ASTNode *node, int indent);

#endif