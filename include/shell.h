#ifndef SHELL_H
#define SHELL_H

void shell_init();
void print_prompt();
char *read_line(char *buffer, int buffer_len);

#endif