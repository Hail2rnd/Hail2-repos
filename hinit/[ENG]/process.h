#ifndef PROCESS_H
#define PROCESS_H


#include <sys/types.h>


pid_t process_spawn(const char *program);

int process_wait(pid_t pid);

int process_signal(pid_t pid, int sig);

pid_t process_reap(int *status);


#endif
