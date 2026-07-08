#ifndef EXECUTE_H
#define EXECUTE_H

#include "parse/parser.h"

typedef struct Shell Shell;

int execute(ASTNode *node, Shell *shell);
char *resolve_path(Shell *shell, const char *command_name);

int execute_command(ASTNode *node, Shell *shell);
int execute_pipe(ASTNode *node, Shell *shell);
int execute_and(ASTNode *node, Shell *shell);

#endif