#ifndef ETERNAL_H
#define ETERNAL_H

#define SHELL_BUFFER_LEN 1024 // reads this many bytes from stdin
#define MAX_ARGS 50 // token_list starting capacity
#define MAX_REDIRECTIONS 20 // max no more than this

#define SHELL_LOCATION "/usr/local/bin/lsh" // where lsh executable is stored

#define SHELL_NAME "lsh"
#define SHELL_VAR_SYMBOL '$'

#define HISTORY_CAPACITY 1024 // historty starting capacity
#define SHELL_VAR_CAPACITY 1024 // shell var starting capacity
#define JOBS_CAPACITY 128 // jobs starting capacity

#endif