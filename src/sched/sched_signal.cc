#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
 
unsigned int pid_1;
unsigned int pid_2;

void handler( int sig ){
  kill(pid_1,SIGCONT);
  kill(pid_2,SIGCONT);
  exit(1);
}

int main( int argc, char* argv[] ) {
  int i;
  pid_1 = atoi( argv[ 1 ] );
  pid_2 = atoi( argv[ 2 ] );
  int flag = 0;


  // Register signals 
  signal( SIGINT, handler ); 
  
  printf( "pid_1: %d, pid_2: %d\n", pid_1, pid_2 );
  	
  while (true) {
     //kill(pid_1,SIGTSTP);
     kill(pid_1,SIGSTOP);
     kill(pid_2,SIGCONT);
     usleep(5);
     //kill(pid_1,SIGTSTP);
     kill(pid_2,SIGSTOP);
     kill(pid_1,SIGCONT);
     usleep(10);
  }

  return 0;
}
