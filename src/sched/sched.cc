#include "sched.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//signal
#ifdef SIGNAL
#include <signal.h>
#include <unistd.h>

int Init_Sched(unsigned int *pid_array) {
	int retval, i;
	retval = 0;
	i = 1;
	while (pid_array[i]) {
		retval = retval & kill(pid_array[i], SIGSTOP);
		i++;
	}
	return retval;
}

int Switch(unsigned int old_pid, unsigned int new_pid) {
	int retval;
	retval = kill(old_pid, SIGSTOP);
	retval = retval & kill(new_pid, SIGCONT);
	return retval;
}

//priority
#else
#include <unistd.h>
#include <sched.h>

struct sched_param sp1 = {
	.sched_priority = 1
};
struct sched_param sp2 = {
	.sched_priority = 2
};

int Init_Sched(unsigned int *pid_array) {
	int retval, i;
	retval = 0;
	i = 1;
	retval = retval & sched_setscheduler(pid_array[0], SCHED_RR, &sp1);
	while (pid_array[i]) {
		retval = retval & sched_setscheduler(pid_array[i], SCHED_RR, &sp2);
		i++;
	}
	return retval;
}

int Switch(unsigned int old_pid, unsigned int new_pid) {
	int retval;
	retval = sched_setscheduler(old_pid, SCHED_RR, &sp2);
	retval = retval & sched_setscheduler(new_pid, SCHED_RR, &sp1);
	return retval;
}

#endif 
