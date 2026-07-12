#include "shell/signal.h"
#include "shell/shell.h"
#include "var/config.h"
#include "var/common.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t sigchild_received;

void sig_shell_init(Shell *shell) {
	shell->joblist.capacity = JOBS_CAPACITY;
	shell->joblist.count = 0;

	shell->joblist.jobs = calloc(shell->joblist.capacity, sizeof(Job));

	sigchild_received = 0;

	if(shell->joblist.jobs == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	signal(SIGCHLD, sigchild_handler);

}

void sig_child_init(Shell *shell) {
	(void)shell;

	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGCONT, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);

}

void add_job(Shell *shell, pid_t pgid, pid_t *pids, int process_count) {
	if(shell->joblist.count >= shell->joblist.capacity) {
		shell->joblist.capacity+=10;
		Job *tmp = realloc(shell->joblist.jobs, sizeof(Job) * shell->joblist.capacity);

		if(!tmp) {
			perror("shell_jobs_realloc");
			return;
		}

		shell->joblist.jobs = tmp;
	}

	Job *job = &shell->joblist.jobs[shell->joblist.count];

	job->id = shell->joblist.count + 1;
	job->pgid = pgid;

	job->process_count = process_count;
	job->finished_count = 0;

	job->pids = malloc(sizeof(pid_t) * job->process_count);

	memcpy(job->pids, pids, process_count * sizeof(pid_t));
	
	job->command = strdup(shell->shell_buffer);
	job->status = JOB_RUNNING;

	shell->joblist.count++;

	printf("|%d| %d\n", job->id, job->pgid);
}

void update_jobs(Shell *shell) {
	if(!sigchild_received) { return; }
	sigchild_received = 0;

	int status;
	pid_t pid;

	while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
		int jobidx = find_job_bypid(shell, pid);
		if(jobidx == -1) { continue; }

		Job *job = &shell->joblist.jobs[jobidx];

		if(WIFEXITED(status) || WIFSIGNALED(status)) {
			job->finished_count++;

			if(job->process_count == job->finished_count) {
				job->status = JOB_DONE;

				printf("|%d| Done (%s)\n", job->id, job->command);

				clean_job(shell, jobidx);
				continue;
			}
		}

		if(WIFSTOPPED(status)) {
			job->status = JOB_STOPPED;
			printf("|%d| Stopped (%s)\n", job->id, job->command);
		}

		if(WIFCONTINUED(status)) {
			job->status = JOB_RUNNING;
			printf("|%d| Continued (%s)\n", job->id, job->command);
		}
	}
}

int find_job_byid(Shell *shell, int id) {
	for(size_t i = 0; i < shell->joblist.count; i++) {
		if(shell->joblist.jobs[i].id == id) {
			return i;
		}
	}
	return -1;
}

int find_job_bypid(Shell *shell, pid_t pid) {
	for(size_t i = 0; i < shell->joblist.count; i++) {
		Job *job = &shell->joblist.jobs[i];
		for(int j = 0; j < job->process_count; j++) {
			if(job->pids[j] == pid) {
				return i;
			}
		}
	}
	return -1;
}

void clean_job(Shell *shell, size_t index) {
	free(shell->joblist.jobs[index].command);
	free(shell->joblist.jobs[index].pids);

	for(size_t i = index; i + 1 < shell->joblist.count; i++) {
		shell->joblist.jobs[i] = shell->joblist.jobs[i+1];
	}

	shell->joblist.count--;
}

void sigchild_handler(int sig) {
	(void)sig;
	sigchild_received = 1;
}