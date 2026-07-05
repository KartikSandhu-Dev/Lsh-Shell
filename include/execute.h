#ifndef EXECUTE_H
#define EXECUTE_H

#include "parser.h"

int execute(ASTNode *node, char **envp);
char *resolve_path(const char *command_name);

int execute_command(ASTNode *node, char **envp);
int execute_pipe(ASTNode *node, char **envp);
int execute_and(ASTNode *node, char **envp);

#endif