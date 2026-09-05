#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


pid_t process_spawn(const char *program)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("hinit: fork");
        return -1;
    }

    if (pid == 0)
    {
        execl(program,
              program,
              (char *)NULL);

        perror("hinit: exec");
        _exit(EXIT_FAILURE);
    }

    return pid;
}


int process_wait(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) < 0)
    {
        perror("hinit: waitpid");
        return -1;
    }

    return status;
}


int process_signal(pid_t pid, int sig)
{
    if (kill(pid, sig) < 0)
    {
        perror("hinit: kill");
        return -1;
    }

    return 0;
}


void process_reap(void)
{
    int status;

    while (waitpid(-1, &status, WNOHANG) > 0)
    {
        /* processos filhos recolhidos */
    }
}
