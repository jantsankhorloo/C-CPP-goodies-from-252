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

    time_t t;
    srand((unsigned) time(&t));

	int res = system("rm -rf tmp/*");
	assert(res >= 0);
	//res = system("mkdir -p tmp");
	//assert(res >= 0);
	
	char command[1024];
	
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
		sprintf(command, "cp %s tmp/%u", argv[r+2], i);
		system(command);
		//dice[i] = r + 2;
		count[r]++;
	}

}
