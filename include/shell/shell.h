#ifndef SHELL_H
#define SHELL_H

void shell_init(char **envp);
void print_prompt();
int read_line(char *buffer, const int buffer_len);

#endif