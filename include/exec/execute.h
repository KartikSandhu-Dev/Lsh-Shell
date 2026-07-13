#ifndef EXECUTE_H
#define EXECUTE_H

#include "parse/parser.h"
#include "var/config.h"

typedef struct Shell Shell;

typedef struct Pipeline {
	ASTNode *commands[PIPELINE_CAPACITY];
	int count;
} Pipeline;

int execute(ASTNode *node, Shell *shell);
char *resolve_path(Shell *shell, const char *command_name);

int execute_command(ASTNode *node, Shell *shell);
int execute_pipeline(Pipeline *pipeline, Shell *shell, bool background);
int execute_and(ASTNode *node, Shell *shell);
int execute_background(ASTNode *node, Shell *shell);

void collect_pipeline(ASTNode *node, Pipeline *pipeline);

#endif