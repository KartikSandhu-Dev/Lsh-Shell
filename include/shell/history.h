#ifndef HISTROY_H
#define HISTROY_H

#include <stdlib.h>

typedef struct Shell Shell;

typedef struct History {
	char **entries;
	size_t count;
	size_t capacity;
} History;

void history_init(Shell *shell);

void history_add(Shell *shell);
char *history_get(Shell *shell, size_t index);

void history_print(Shell *shell);
void history_clear(Shell *shell);

#endif