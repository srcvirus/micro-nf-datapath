#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
 
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

        unsigned SHARE_1 = 1;
        unsigned SHARE_2 = 2;
        int r = 1;
        int m = 1;
        SHARE_1 = (unsigned) ( m * SHARE_1 / r);
        SHARE_2 = (unsigned) ( m * SHARE_2 / r);
        std::cout << "!!!" << SHARE_1 << " " << SHARE_2 << std::endl;
        
        // Register signals 
        signal(SIGINT, handler); 
  
        // Priority the higher the better  
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
           // Share for pid_1
           sched_setscheduler(pid_1, SCHED_RR, &sp2);
           sched_setscheduler(pid_2, SCHED_RR, &sp1);
           usleep(SHARE_1);
           //nanosleep(&duration, NULL);


           // Share for pid_2
           sched_setscheduler(pid_2, SCHED_RR, &sp2);
           sched_setscheduler(pid_1, SCHED_RR, &sp1);
           usleep(SHARE_2);
           //nanosleep(&duration, NULL);
	}

  	return 0;
}
