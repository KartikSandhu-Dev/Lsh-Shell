#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdlib.h>

typedef struct Shell Shell;

typedef enum JobStatus {
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE
} JobStatus;

typedef struct Job {
	int id;
	int pid;
	char *command;
	JobStatus status;
} Job;

typedef struct JobList {
	Job *jobs;
	size_t count;
	size_t capacity;
	size_t next_id;
} JobList;

void sig_shell_init(Shell *shell);
void sig_child_init(Shell *shell);

void add_job(Shell *shell, int pid);
void check_jobs(Shell *shell);

void clean_job(Shell *shell, size_t index);

#endif