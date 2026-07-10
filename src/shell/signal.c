#include "shell/signal.h"
#include "shell/shell.h"
#include "var/config.h"
#include "var/common.h"
#include <bits/types/idtype_t.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

void sig_shell_init(Shell *shell) {
	shell->joblist.capacity = JOBS_CAPACITY;
	shell->joblist.count = 0;

	shell->joblist.jobs = calloc(shell->joblist.capacity, sizeof(Job) * shell->joblist.capacity);
}

void sig_child_init(Shell *shell) {

}

void add_job(Shell *shell, int pid) {
	if(shell->joblist.count >= shell->joblist.capacity) {
		shell->joblist.capacity+=10;
		Job *tmp = realloc(shell->joblist.jobs, sizeof(Job) * shell->joblist.capacity);

		if(!tmp) {
			perror("shell_jobs_realloc");
			return;
		}

		shell->joblist.jobs = tmp;
	}

	Job job;
	job.id = shell->joblist.count + 1;
	job.pid = pid;
	job.command = strdup(shell->shell_buffer);
	job.status = JOB_RUNNING;

	shell->joblist.jobs[shell->joblist.count] = job;
	shell->joblist.count++;

	printf("|%d| %d\n", job.id, job.pid);
}

void check_jobs(Shell *shell) {
	int status;

	for(size_t i = 0; i < shell->joblist.count; i++) {
		Job *job = &shell->joblist.jobs[i];

		pid_t result = waitpid(job[i].pid, 
				&status, 
				WNOHANG);

		if(result == 0) { continue; }

		if(result == job->pid) {
			job->status = JOB_DONE;

			printf("|%d| done %s\n", job->id, job->command);

			clean_job(shell, i);
		}
	}

}

void clean_job(Shell *shell, size_t index) {
	free(shell->joblist.jobs[index].command);

	for(size_t i = index; i + 1 < shell->joblist.count; i++) {
		shell->joblist.jobs[i] = shell->joblist.jobs[i+1];
	}

	shell->joblist.count--;
}