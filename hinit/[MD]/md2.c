#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "../[LOG]/log.h"


static volatile sig_atomic_t running = 1;


static void md2_signal(int sig)
{
    switch (sig)
    {
        case SIGTERM:
        case SIGINT:
            running = 0;
            break;

        default:
            break;
    }
}


static int md2_check_hinit(void)
{
    pid_t parent;


    parent = getppid();


    if (parent != 1)
    {
        log_write(
            "MD2",
            "HINIT IS NO LONGER PID1",
            LOG_LEVEL_FATAL
        );


        fprintf(
            stderr,
            "[MD2] Critical: HInit is no longer PID1\n"
        );


        return -1;
    }


    if (kill(
            1,
            0
        ) < 0)
    {
        if (errno == ESRCH)
        {
            log_write(
                "MD2",
                "HINIT PID1 DISAPPEARED",
                LOG_LEVEL_FATAL
            );


            fprintf(
                stderr,
                "[MD2] Critical: HInit/PID1 disappeared\n"
            );
        }
        else if (errno == EPERM)
        {
            log_write(
                "MD2",
                "CANNOT INSPECT HINIT PID1",
                LOG_LEVEL_WARN
            );


            fprintf(
                stderr,
                "[MD2] Warning: cannot inspect HInit/PID1\n"
            );
        }
        else
        {
            log_write(
                "MD2",
                "HINIT PID1 CHECK FAILED",
                LOG_LEVEL_FATAL
            );


            perror(
                "[MD2] Critical: HInit/PID1 check"
            );


            return -1;
        }
    }


    return 0;
}


static int md2_check_runtime(void)
{
    struct stat st;


    if (stat(
            "/run/hinit",
            &st
        ) < 0)
    {
        if (errno == ENOENT)
        {
            log_write(
                "MD2",
                "/RUN/HINIT DISAPPEARED",
                LOG_LEVEL_FATAL
            );


            fprintf(
                stderr,
                "[MD2] Critical: /run/hinit disappeared\n"
            );
        }
        else
        {
            log_write(
                "MD2",
                "/RUN/HINIT CHECK FAILED",
                LOG_LEVEL_FATAL
            );


            perror(
                "[MD2] Critical: /run/hinit check"
            );
        }


        return -1;
    }


    if (!S_ISDIR(st.st_mode))
    {
        log_write(
            "MD2",
            "/RUN/HINIT IS NOT A DIRECTORY",
            LOG_LEVEL_FATAL
        );


        fprintf(
            stderr,
            "[MD2] Critical: /run/hinit is not a directory\n"
        );


        return -1;
    }


    return 0;
}


static int md2_check(void)
{
    if (md2_check_hinit() < 0)
    {
        return -1;
    }


    if (md2_check_runtime() < 0)
    {
        return -1;
    }


    return 0;
}


int main(void)
{
    struct sigaction sa;


    if (log_init() < 0)
    {
        fprintf(
            stderr,
            "[MD2] Failed to initialize logger\n"
        );


        return EXIT_FAILURE;
    }


    log_write(
        "MD2",
        "SYSTEM OPERATING",
        LOG_LEVEL_LOG
    );


    sigemptyset(&sa.sa_mask);

    sa.sa_handler = md2_signal;

    sa.sa_flags = 0;


    if (sigaction(
            SIGTERM,
            &sa,
            NULL
        ) < 0)
    {
        log_write(
            "MD2",
            "SIGACTION SIGTERM FAILED",
            LOG_LEVEL_FATAL
        );


        perror(
            "[MD2] sigaction SIGTERM"
        );


        return EXIT_FAILURE;
    }


    if (sigaction(
            SIGINT,
            &sa,
            NULL
        ) < 0)
    {
        log_write(
            "MD2",
            "SIGACTION SIGINT FAILED",
            LOG_LEVEL_FATAL
        );


        perror(
            "[MD2] sigaction SIGINT"
        );


        return EXIT_FAILURE;
    }


    while (running)
    {
        if (md2_check() < 0)
        {
            log_write(
                "MD2",
                "CRITICAL OPERATIONAL FAILURE",
                LOG_LEVEL_FATAL
            );


            fprintf(
                stderr,
                "[MD2] Critical operational failure\n"
            );


            return EXIT_FAILURE;
        }


        sleep(1);
    }


    log_write(
        "MD2",
        "LEAVING OPERATING MODE",
        LOG_LEVEL_LOG
    );


    return EXIT_SUCCESS;
}
