#ifndef _SCHED_H_
#define _SCHED_H_
//#define SIGNAL 1

/* Initilize an array of pids, send SIGSTOP or set priority. 
The first pid in the array will continue run and others will be blocked*/
int Init_Sched(unsigned int *pid_array);

/* Suspend the old_pid and resume the new_pid */
int Switch(unsigned int old_pid, unsigned int new_pid);

#endif
