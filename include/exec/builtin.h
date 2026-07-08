#ifndef BUILTIN_H
#define BUILTIN_H

#include "parse/parser.h"

typedef struct Shell Shell;

typedef struct BuiltIn {
	const char *name;
	int (*func)(ASTNode *node, Shell *shell);
} BuiltIn;

BuiltIn find_builtin(char *command);

int cd_builtin(ASTNode *node, Shell *shell);
int exit_builtin(ASTNode *node, Shell *shell);
int pwd_builtin(ASTNode *node, Shell *shell);
int echo_builtin(ASTNode *node, Shell *shell);
int export_builtin(ASTNode *node, Shell *shell);
int unset_builtin(ASTNode *node, Shell *shell);
int history_builtin(ASTNode *node, Shell *shell);

#endif