#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
 
unsigned int pid_1;
unsigned int pid_2;

void handler( int sig ){
   struct sched_param sp1 = {
      .sched_priority = 99
   };

   sched_setscheduler(pid_2, SCHED_RR, &sp1);
   sched_setscheduler(pid_1, SCHED_RR, &sp1);

   exit(1);
}


int main( int argc, char* argv[] ) {
	int i;
	pid_1 = atoi( argv[ 1 ] );
	pid_2 = atoi( argv[ 2 ] );

        // Register signals 
        signal(SIGINT, handler); 
  
        // Priority the lower the better  
        // 1 to 99
	struct sched_param sp1 = {
		.sched_priority = 1
	};
	struct sched_param sp2 = {
		.sched_priority = 2
	};

	struct timespec duration;
	duration.tv_sec = 0;        /* seconds */
	duration.tv_nsec = 0;       /* nanoseconds */
       
	
	while (true) {
		sched_setscheduler(pid_1, SCHED_RR, &sp2);
		sched_setscheduler(pid_2, SCHED_RR, &sp1);
		usleep(2);
		//nanosleep(&duration, NULL);
		sched_setscheduler(pid_2, SCHED_RR, &sp2);
		sched_setscheduler(pid_1, SCHED_RR, &sp1);
		usleep(2);
		//nanosleep(&duration, NULL);
	}

  	return 0;
}
