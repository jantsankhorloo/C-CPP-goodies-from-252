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

void child_init(int i, char *prog, char *nproc, char *nprog)
{
    char *args[4];

    struct timespec cur_time, sleep_time, rem_time;
    pid_t    pid = getpid();

    setpgid(0,0);     // change process group id to prevent being killed in one group

    // set the priority to be low so that the system can better react to observation and cleanup
    if (setpriority(PRIO_PROCESS, 0, 19) < 0) 
        perror("SETPRIORITY"); 

    // sleep until about 2 seconds from start_time
    clock_gettime(CLOCK_REALTIME, &cur_time);
    sleep_time.tv_sec = start_time.tv_sec + 2 - cur_time.tv_sec;
    sleep_time.tv_nsec = start_time.tv_nsec - cur_time.tv_nsec;
    if (sleep_time.tv_nsec < 0) {
        sleep_time.tv_sec--;
        sleep_time.tv_nsec += 1e9;
    }
    if (sleep_time.tv_sec >=0) {
        nanosleep(&sleep_time, &rem_time);
    }

    fprintf(stderr, "Process %d loading %s \n", pid, prog);
    args[0]=prog;
    args[1]=nproc;
    args[2]=nprog;
    args[3]=NULL;
    execvp(args[0], args);
    perror("EXEC: ");
    _exit(1);
}

int main(int argc, char *argv[])
{
    if (argc < 4 || argc > 7) {
        fprintf(stderr, "Usage: arena num_processes_per_prog prog1 prog2 [prog3 ... prog5]\n");
        _exit(0);
    }

    int num = atoi(argv[1]);
    if (num<10) {
        fprintf(stderr, "The number of processes for each prog should be at least 10.\n");
        _exit(0);
    }

    int np = argc - 2;
    char snprog[10];
    sprintf(snprog, "%d", np);
    int total = num * np;

    if (total > MAX_WARRIORS) {
        fprintf(stderr, "The total number of processes should be at most %d.\n", MAX_WARRIORS);
        _exit(0);
    }

    // block all signals to the extent allowed by the kernel, inherited by all children
    sigset_t   mask;
    sigfillset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    struct rlimit lim;
    lim.rlim_cur = total+2;
    lim.rlim_max = total+2;
    if (setrlimit(RLIMIT_NPROC, &lim) < 0)  {
        perror("SETRLIMIT");
        _exit(1);
    }

    // Set up the dice array to randomly assign processes to programs.
    int dice[MAX_WARRIORS];
    int count[5];
    int i;
    for (i=0; i<np; i++) {
        count[i] = 0;
    }
    for (i=0; i<total; i++) {
        int r;
        do {
            r = rand() % np;
        } while (count[r] == num);
        dice[i] = r + 2;
        count[r]++;
    }

    clock_gettime(CLOCK_REALTIME, &start_time);

    for (i=0; i<total; i++) {
        pid_t cid = fork();
        if (cid < 0) {
            fprintf(stderr, "Creating %d processes failed at number %d.\n", total-1, i+1);
            _exit(1);
        } else if (cid == 0) {  // child
            child_init(i, argv[dice[i]], argv[1],snprog);
        } 
    }
    // the arena itself needs to exit
}
