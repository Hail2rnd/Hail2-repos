#define _POSIX_C_SOURCE 200809L

#include "process.h"

#include "../[LOG]/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


static void process_log_error(
    const char *message,
    int level
)
{
    if (log_init() == 0)
    {
        log_write(
            "PROCESS",
            message,
            level
        );
    }
}


pid_t process_spawn(
    const char *program
)
{
    pid_t pid;


    if (program == NULL)
    {
        process_log_error(
            "PROCESS SPAWN INVALID PROGRAM",
            LOG_LEVEL_CRITICAL
        );


        fprintf(
            stderr,
            "hinit: process_spawn: invalid program\n"
        );


        return -1;
    }


    pid = fork();


    if (pid < 0)
    {
        process_log_error(
            "FORK FAILED",
            LOG_LEVEL_CRITICAL
        );


        perror(
            "hinit: fork"
        );


        return -1;
    }


    if (pid == 0)
    {
        execl(
            program,
            program,
            (char *)NULL
        );


        /*
         * This is the child process. The parent
         * Logger state may exist, but this child
         * is about to terminate immediately.
         */
        fprintf(
            stderr,
            "hinit: exec: %s\n",
            program
        );


        _exit(EXIT_FAILURE);
    }


    return pid;
}


int process_wait(
    pid_t pid
)
{
    int status;


    if (pid <= 0)
    {
        process_log_error(
            "PROCESS WAIT INVALID PID",
            LOG_LEVEL_CRITICAL
        );


        fprintf(
            stderr,
            "hinit: process_wait: invalid pid\n"
        );


        return -1;
    }


    for (;;)
    {
        if (waitpid(
                pid,
                &status,
                0
            ) >= 0)
        {
            return status;
        }


        if (errno == EINTR)
        {
            continue;
        }


        process_log_error(
            "WAITPID FAILED",
            LOG_LEVEL_CRITICAL
        );


        perror(
            "hinit: waitpid"
        );


        return -1;
    }
}


int process_signal(
    pid_t pid,
    int sig
)
{
    if (pid <= 0)
    {
        process_log_error(
            "PROCESS SIGNAL INVALID PID",
            LOG_LEVEL_CRITICAL
        );


        fprintf(
            stderr,
            "hinit: process_signal: invalid pid\n"
        );


        return -1;
    }


    if (sig <= 0)
    {
        process_log_error(
            "PROCESS SIGNAL INVALID SIGNAL",
            LOG_LEVEL_CRITICAL
        );


        fprintf(
            stderr,
            "hinit: process_signal: invalid signal\n"
        );


        return -1;
    }


    if (kill(
            pid,
            sig
        ) < 0)
    {
        if (errno == ESRCH)
        {
            return -1;
        }


        process_log_error(
            "KILL FAILED",
            LOG_LEVEL_CRITICAL
        );


        perror(
            "hinit: kill"
        );


        return -1;
    }


    return 0;
}


pid_t process_reap(
    int *status
)
{
    pid_t pid;


    if (status == NULL)
    {
        process_log_error(
            "PROCESS REAP INVALID STATUS POINTER",
            LOG_LEVEL_CRITICAL
        );


        fprintf(
            stderr,
            "hinit: process_reap: invalid status pointer\n"
        );


        return -1;
    }


    for (;;)
    {
        pid = waitpid(
            -1,
            status,
            WNOHANG
        );


        if (pid > 0)
        {
            return pid;
        }


        if (pid == 0)
        {
            return 0;
        }


        if (errno == EINTR)
        {
            continue;
        }


        if (errno == ECHILD)
        {
            return 0;
        }


        process_log_error(
            "PROCESS REAP WAITPID FAILED",
            LOG_LEVEL_CRITICAL
        );


        perror(
            "hinit: waitpid"
        );


        return -1;
    }
}
