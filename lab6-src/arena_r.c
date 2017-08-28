#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define MAX_WARRIORS 100

static struct timespec  start_time;

void child_init(int i)
{
    char *args[4];
	char command[512];

    sprintf(command, "tmp/%u", i);

    struct timespec cur_time, sleep_time, rem_time;
	pid_t  pid = getpid();

    setpgid(0,0);	 // change process group id to prevent being killed in one group

	// set the priority to be low so that the system can better react to observation and cleanup
	if (setpriority(PRIO_PROCESS, 0, 19) < 0) 
		perror("SETPRIORITY"); 

    args[0]=command;
	args[1]=NULL;
	
	// sleep until about 5 seconds from start_time
	clock_gettime(CLOCK_REALTIME, &cur_time);
    sleep_time.tv_sec = start_time.tv_sec + 5 - cur_time.tv_sec;
    sleep_time.tv_nsec = start_time.tv_nsec - cur_time.tv_nsec;
    if (sleep_time.tv_nsec < 0) {
        sleep_time.tv_sec--;
        sleep_time.tv_nsec += 1e9;
    }
	if (sleep_time.tv_sec >=0) {
		nanosleep(&sleep_time, &rem_time);
	}

    fprintf(stderr, "Process %d loading %s \n", pid, command);
    execvp(args[0], args);
	perror("EXEC: ");
	_exit(1);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: arena_r total_num_processes\n");
		_exit(0);
	}

	int total = atoi(argv[1]);
	if (total > MAX_WARRIORS) {
		fprintf(stderr, "The total number of processes should be at most %d.\n", MAX_WARRIORS);
		_exit(0);
	}

    time_t t;
    srand((unsigned) time(&t));

	// block all signals to the extent allowed by the kernel, inherited by all children
	sigset_t   mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

    struct rlimit lim;
    lim.rlim_cur = total+1;
    lim.rlim_max = total+1;
	if (setrlimit(RLIMIT_NPROC, &lim) < 0)  {
		perror("SETRLIMIT");
		_exit(1);
	}

	clock_gettime(CLOCK_REALTIME, &start_time);

	int i;
	for (i=0; i<total; i++) {
		pid_t cid = fork();
		if (cid < 0) {
			fprintf(stderr, "Creating %d processes failed at number %d.\n", total-1, i+1);
			_exit(1);
		} else if (cid == 0) {  // child
			child_init(i);
		} 
	}
	// the arena itself needs to exit
}
