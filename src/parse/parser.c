#include "parse/lexer.h"
#include "parse/parser.h"
#include "var/colors.h"
#include "var/config.h"
#include "var/common.h"
#include <stdio.h>
#include <stdlib.h>

static Token current(Parser *p) {
	return p->token_list.tokens[p->pos];
}

static void advance(Parser *p) {
	p->pos++;
}

ASTNode *parse_tokens(TokenList *token_list) {
	Parser p = {0};
	p.token_list = *token_list;
	p.pos = 0;

	return parse_background(&p);
}

static void free_command(ASTNode *node) {
	size_t pos = 0;
	while(1) {
		free(node->Command.argv[pos]);
		if(node->Command.argv[pos] == NULL) {
			break;
		}
		pos++;
	}

	pos = 0;
	while(pos < node->Command.redir_count) {
		free(node->Command.redirs[pos].file);
		pos++;
	}

	free(node->Command.argv);
	free(node->Command.redirs);
	free(node);
}

ASTNode *parse_command(Parser *p) {
	ASTNode *node = calloc(1, sizeof(ASTNode));

	node->ast_type = NODE_COMMAND;
	node->Command.argc = 0;
	node->Command.argv = malloc(sizeof(char *)*MAX_ARGS);

	node->Command.redirs = malloc(sizeof(Redir)*MAX_REDIRECTIONS);
	node->Command.redir_count = 0;

	while(current(p).token_type == TOKEN_WORD) {
		if(node->Command.argc >= MAX_ARGS - 1) {
			fprintf(stderr, BR_RED "Too many arguments" RESET "\n");
			free_command(node);
			return NULL;
		}

		node->Command.argv[node->Command.argc] = strdup(current(p).value);
		node->Command.argc++;
		advance(p);
	}

	// ["ls", "-la", NULL] 
	node->Command.argv[node->Command.argc] = NULL;

	while(current(p).token_type == TOKEN_REDIR_IN || 
		  current(p).token_type == TOKEN_REDIR_OUT ||
		  current(p).token_type == TOKEN_APPEND) {

		if(node->Command.redir_count >= MAX_REDIRECTIONS) {
			fprintf(stderr, BR_RED "Too many REDIRECTIONS" RESET "\n");
			free_command(node);
			return NULL;
		}
		Redir r = {0};

		switch (current(p).token_type) {
			case TOKEN_REDIR_IN:
				r.type = REDIR_IN;
				break;
			case TOKEN_REDIR_OUT:
				r.type = REDIR_OUT;
				break;
			case TOKEN_APPEND:
				r.type = REDIR_APPEND;
				break;
			default:
				break;
        }

        advance(p);

        if(current(p).token_type == TOKEN_WORD) {
        	r.file = strdup(current(p).value);
        } else {
        	fprintf(stderr, BR_RED "No filepath after redirection" RESET "\n");
        	return NULL;
        }

        advance(p);

        node->Command.redirs[node->Command.redir_count] = r;
        node->Command.redir_count++;
    }

	return node;
}

ASTNode *parse_and(Parser *p) {
	ASTNode *left = parse_pipe(p);

	while(current(p).token_type == TOKEN_AND) {
		advance(p);

		ASTNode *right = parse_pipe(p);

		ASTNode *node = calloc(1, sizeof(ASTNode));
		node->ast_type = NODE_AND;

		node->Binary.left = left;
		node->Binary.right = right;

		left = node;
	}

	return left;
}

ASTNode *parse_pipe(Parser *p) {
	ASTNode *left = parse_command(p);

	while(current(p).token_type == TOKEN_PIPE) {
		advance(p);

		ASTNode *right = parse_command(p);

		ASTNode *node = calloc(1, sizeof(ASTNode));
		node->ast_type = NODE_PIPE;

		node->Binary.left = left;
		node->Binary.right = right;

		left = node;
	}

	return left;
}

ASTNode *parse_background(Parser *p) {
	ASTNode *node = parse_and(p);

	if(current(p).token_type == TOKEN_BACKGROUND) {
		ASTNode *bg = calloc(1, sizeof(ASTNode));
		bg->ast_type = NODE_BACKGROUND;

		bg->Background.node = node;

		node = bg;
	}

	return node;
}

void clean_ASTs(ASTNode *node) {
	if(!node) { return; }

	switch (node->ast_type) {
		case NODE_COMMAND:
			free_command(node);
			break;
		case NODE_PIPE:
			clean_ASTs(node->Binary.left);
			clean_ASTs(node->Binary.right);
			free(node);
			break;
		case NODE_AND:
			clean_ASTs(node->Binary.left);
			clean_ASTs(node->Binary.right);
			free(node);
			break;
		case NODE_BACKGROUND:
			clean_ASTs(node->Background.node);
			free(node);
			break;
		}
}

static void print_indent(int indent) {
	for(int i = 0; i < indent; i++) {
		printf("   ");
	}

}

void print_ASTs(ASTNode *node, int indent) {
	if(!node) { return; }

	print_indent(indent);

	switch (node->ast_type) {
		case NODE_COMMAND:
			printf("Command\n");
			// printf("Command\n");
			print_indent(indent + 1);
			printf("argv:\n");

			size_t pos = 0;
			while(node->Command.argv[pos] != NULL) {
				print_indent(indent + 2);
				printf("%s\n", node->Command.argv[pos]);
				pos++;
			}
			break;
		case NODE_PIPE:
			printf("Pipe\n");
			print_ASTs(node->Binary.left, indent + 1);
			print_ASTs(node->Binary.right, indent + 1);
			break;
		case NODE_AND:
			printf("And\n");
			print_ASTs(node->Binary.left, indent + 1);
			print_ASTs(node->Binary.right, indent + 1);
			break;
		case NODE_BACKGROUND:
			printf("Background\n");
			print_ASTs(node->Background.node, indent +1);
			break;
		}
}
