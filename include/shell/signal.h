#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdlib.h>
#include <signal.h>

typedef struct Shell Shell;

typedef enum JobStatus {
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE
} JobStatus;

typedef struct Job {
	int id;
	pid_t pgid;

	pid_t *pids;
	int process_count;
	int finished_count;

	char *command;
	JobStatus status;
} Job;

typedef struct JobList {
	Job *jobs;
	size_t count;
	size_t capacity;
} JobList;

extern volatile sig_atomic_t sigchild_received;

void sig_shell_init(Shell *shell);
void sig_child_init(Shell *shell);

void add_job(Shell *shell, pid_t pgid, pid_t *pids, int process_count);
void update_jobs(Shell *shell);
void check_jobs(Shell *shell);

int find_job_byid(Shell *shell, int id);
int find_job_bypid(Shell *shell, pid_t pid);
void clean_job(Shell *shell, size_t index);

void sigchild_handler(int sig);

#endif