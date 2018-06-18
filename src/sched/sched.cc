#include "sched.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//signal
#ifdef SIGNAL
#include <signal.h>
#include <unistd.h>

unsigned int pids[ 100 ];

void Handler(){
   int i = 0;
   while( pids[i]) {
      kill( pids[ i ], SIGCONT);
      i++;
   }
}

int Init_Sched(unsigned int *pid_array) {
	int retval, i;
	retval = 0;
	i = 0;
	while (pid_array[i]) {
		retval = retval | kill(pid_array[i], SIGSTOP);
                pids[ i ] = pid_array[ i ];
		i++;
	}
	return retval;
}

int Switch(unsigned int old_pid, unsigned int new_pid) {
	int retval;
        if ( old_pid )
           retval = kill(old_pid, SIGSTOP);
	retval = retval | kill(new_pid, SIGCONT);
	return retval;
}

//priority
#else
#include <unistd.h>
#include <sched.h>

unsigned int pids[ 100 ];

struct sched_param sp1 = {
	.sched_priority = 1
};
struct sched_param sp2 = {
	.sched_priority = 2
};

int Init_Sched(unsigned int *pid_array) {
	int retval, i;
	retval = 0;
	i = 0;
	while (pid_array[i]) {
		retval = retval | sched_setscheduler(pid_array[i], SCHED_RR, &sp1);
                pids[ i ] = pid_array[ i ];
		i++; 
	}
	return retval;
}

int Switch(unsigned int old_pid, unsigned int new_pid) {
	int retval;

        if ( old_pid )
           retval = sched_setscheduler(old_pid, SCHED_RR, &sp1);
	retval = retval | sched_setscheduler(new_pid, SCHED_RR, &sp2);
	return retval;
}

void Handler() {
   struct sched_param sp1 = {
      .sched_priority = 99
   };
   int i = 0;
   while( pids[i] ) {
      sched_setscheduler(pids[i], SCHED_RR, &sp1);
      i++;
   }
}

#endif 
