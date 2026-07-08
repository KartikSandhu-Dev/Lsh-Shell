#include "shell/history.h"
#include "var/config.h"
#include "var/common.h"
#include "shell/shell.h"

void history_init(Shell *shell) {
	shell->history.count = 0;
	shell->history.capacity = HISTORY_CAPACITY;
	shell->history.entries = malloc(sizeof(char *)*shell->history.capacity);
}

void history_add(Shell *shell) {
	if(shell->history.count >= shell->history.capacity) {
		shell->history.capacity += 50;
		shell->history.entries = realloc(shell->history.entries, sizeof(char *)*shell->history.capacity);
	}

	shell->history.entries[shell->history.count] = strdup(shell->shell_buffer);
	shell->history.count++;
}

char *history_get(Shell *shell, size_t index) {
	if(index > shell->history.capacity) {
		return NULL;
	}
	return shell->history.entries[index];
}

void history_print(Shell *shell) {
	for(size_t i = 0; i < shell->history.count; i++) {
		printf("%zu. %s\n", i, shell->history.entries[i]);
	}
}


void history_clean(Shell *shell) {
	for(size_t i = 0; i < shell->history.count; i++) {
		free(shell->history.entries[i]);
	}

	free(shell->history.entries);

	shell->history.count = 0;
	shell->history.capacity = 0;
}