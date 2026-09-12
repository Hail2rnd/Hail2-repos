#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#include "../[LOG]/log.h"

#include "process.h"


#define HSV_STOP_TIMEOUT 5


static volatile sig_atomic_t shutting_down = 0;
static volatile sig_atomic_t restart_requested = 0;


/*
 * HSV only records control requests here.
 *
 * No process operations are performed inside
 * the signal handler.
 */
static void hsv_signal(int sig)
{
    switch (sig)
    {
        case SIGTERM:
            shutting_down = 1;
            break;

        case SIGUSR1:
            restart_requested = 1;
            break;

        default:
            break;
    }
}


/*
 * Build a service script path safely.
 *
 * Returns:
 *
 *     0  -> success
 *    -1  -> invalid/truncated path
 */
static int hsv_build_path(
    char *buffer,
    size_t buffer_size,
    const char *service,
    const char *script
)
{
    int length;


    if (buffer == NULL ||
        buffer_size == 0 ||
        service == NULL ||
        script == NULL)
    {
        return -1;
    }


    length = snprintf(
        buffer,
        buffer_size,
        "%s/%s",
        service,
        script
    );


    if (length < 0)
    {
        return -1;
    }


    if ((size_t)length >= buffer_size)
    {
        return -1;
    }


    return 0;
}


/*
 * Execute one optional service script.
 *
 * A missing/non-executable script is not an
 * HSV failure. It simply means that script
 * does not exist for this service.
 */
static int hsv_exec_script(const char *path)
{
    pid_t pid;
    int status;


    if (path == NULL)
    {
        return -1;
    }


    if (access(path, X_OK) != 0)
    {
        if (errno == ENOENT ||
            errno == ENOTDIR ||
            errno == EACCES)
        {
            return 0;
        }


        return -1;
    }


    pid = process_spawn(path);


    if (pid < 0)
    {
        return -1;
    }


    status = process_wait(pid);


    if (status < 0)
    {
        return -1;
    }


    return status;
}


/*
 * Sleep for a small amount of time.
 *
 * nanosleep() may be interrupted by signals,
 * which is useful while HSV is supervising
 * the service.
 */
static void hsv_sleep_ms(long milliseconds)
{
    struct timespec request;
    struct timespec remaining;


    request.tv_sec =
        milliseconds / 1000;


    request.tv_nsec =
        (milliseconds % 1000) * 1000000L;


    while (nanosleep(
        &request,
        &remaining
    ) < 0)
    {
        if (errno != EINTR)
        {
            return;
        }


        if (shutting_down ||
            restart_requested)
        {
            return;
        }


        request = remaining;
    }
}


/*
 * Stop the currently supervised service process.
 *
 * The supervisor first sends SIGTERM and gives
 * the service a limited amount of time to exit.
 *
 * If it refuses to terminate, SIGKILL is used
 * so HSV itself cannot remain permanently stuck.
 */
static void hsv_stop_service(pid_t pid)
{
    int status;
    int elapsed;


    if (pid <= 0)
    {
        return;
    }


    if (kill(
            pid,
            SIGTERM
        ) < 0)
    {
        if (errno == ESRCH)
        {
            return;
        }
    }


    for (elapsed = 0;
         elapsed < HSV_STOP_TIMEOUT * 10;
         elapsed++)
    {
        pid_t result;


        result = waitpid(
            pid,
            &status,
            WNOHANG
        );


        if (result == pid)
        {
            return;
        }


        if (result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }


            if (errno == ECHILD)
            {
                return;
            }


            return;
        }


        hsv_sleep_ms(100);
    }


    /*
     * The service did not stop within the
     * configured timeout.
     */
    log_write(
        "HSV",
        "SERVICE DID NOT STOP, USING SIGKILL",
        LOG_LEVEL_WARN
    );


    if (kill(
            pid,
            SIGKILL
        ) < 0)
    {
        if (errno == ESRCH)
        {
            return;
        }
    }


    for (;;)
    {
        if (waitpid(
                pid,
                &status,
                0
            ) >= 0)
        {
            return;
        }


        if (errno == EINTR)
        {
            continue;
        }


        return;
    }
}


int main(int argc, char *argv[])
{
    char run_path[PATH_MAX];
    char stop_path[PATH_MAX];
    char finish_path[PATH_MAX];

    struct sigaction sa;


    if (argc != 2)
    {
        log_write(
            "HSV",
            "INVALID ARGUMENT COUNT",
            LOG_LEVEL_FATAL
        );


        fprintf(
            stderr,
            "Usage: %s <service>\n",
            argv[0]
        );


        return EXIT_FAILURE;
    }


    if (hsv_build_path(
            run_path,
            sizeof(run_path),
            argv[1],
            "run"
        ) < 0)
    {
        log_write(
            "HSV",
            "RUN PATH TOO LONG",
            LOG_LEVEL_FATAL
        );


        return EXIT_FAILURE;
    }


    if (hsv_build_path(
            stop_path,
            sizeof(stop_path),
            argv[1],
            "stop"
        ) < 0)
    {
        log_write(
            "HSV",
            "STOP PATH TOO LONG",
            LOG_LEVEL_FATAL
        );


        return EXIT_FAILURE;
    }


    if (hsv_build_path(
            finish_path,
            sizeof(finish_path),
            argv[1],
            "finish"
        ) < 0)
    {
        log_write(
            "HSV",
            "FINISH PATH TOO LONG",
            LOG_LEVEL_FATAL
        );


        return EXIT_FAILURE;
    }


    /*
     * Configure HSV control signals.
     */
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = hsv_signal;

    sa.sa_flags = 0;


    if (sigaction(
            SIGTERM,
            &sa,
            NULL
        ) < 0)
    {
        log_write(
            "HSV",
            "SIGACTION SIGTERM FAILED",
            LOG_LEVEL_FATAL
        );


        perror(
            "hsv: sigaction SIGTERM"
        );


        return EXIT_FAILURE;
    }


    if (sigaction(
            SIGUSR1,
            &sa,
            NULL
        ) < 0)
    {
        log_write(
            "HSV",
            "SIGACTION SIGUSR1 FAILED",
            LOG_LEVEL_FATAL
        );


        perror(
            "hsv: sigaction SIGUSR1"
        );


        return EXIT_FAILURE;
    }


    {
        char message[256];


        snprintf(
            message,
            sizeof(message),
            "SUPERVISING SERVICE %s",
            argv[1]
        );


        log_write(
            "HSV",
            message,
            LOG_LEVEL_LOG
        );
    }


    /*
     * Main supervision loop.
     */
    for (;;)
    {
        pid_t pid;
        int status;


        if (shutting_down)
        {
            break;
        }


        /*
         * Start the service's run script.
         */
        pid = process_spawn(run_path);


        if (pid < 0)
        {
            log_write(
                "HSV",
                "FAILED TO START SERVICE",
                LOG_LEVEL_CRITICAL
            );


            hsv_sleep_ms(1000);

            continue;
        }


        /*
         * Supervise the running service.
         */
        for (;;)
        {
            pid_t result;


            result = waitpid(
                pid,
                &status,
                WNOHANG
            );


            if (result == pid)
            {
                /*
                 * Service exited.
                 */
                break;
            }


            if (result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }


                if (errno == ECHILD)
                {
                    break;
                }


                log_write(
                    "HSV",
                    "WAITPID FAILED WHILE SUPERVISING SERVICE",
                    LOG_LEVEL_CRITICAL
                );


                break;
            }


            /*
             * Explicit restart request.
             */
            if (restart_requested)
            {
                restart_requested = 0;


                log_write(
                    "HSV",
                    "SERVICE RESTART REQUESTED",
                    LOG_LEVEL_LOG
                );


                hsv_stop_service(pid);


                /*
                 * Execute the service's stop hook
                 * before starting it again.
                 */
                if (hsv_exec_script(stop_path) < 0)
                {
                    log_write(
                        "HSV",
                        "SERVICE STOP HOOK FAILED",
                        LOG_LEVEL_WARN
                    );
                }


                break;
            }


            /*
             * Shutdown request.
             */
            if (shutting_down)
            {
                hsv_stop_service(pid);

                break;
            }


            hsv_sleep_ms(100);
        }


        if (shutting_down)
        {
            break;
        }


        /*
         * Restart was explicitly requested.
         *
         * Do not wait the normal crash-restart delay.
         */
        if (restart_requested)
        {
            continue;
        }


        /*
         * The service exited while the system was
         * operating. HSV supervises it and restarts
         * it after a short delay.
         */
        log_write(
            "HSV",
            "SERVICE EXITED, RESTARTING",
            LOG_LEVEL_WARN
        );


        hsv_sleep_ms(1000);
    }


    /*
     * HSV itself is shutting down.
     *
     * stop runs first, then finish.
     */
    log_write(
        "HSV",
        "SUPERVISOR SHUTTING DOWN",
        LOG_LEVEL_LOG
    );


    if (hsv_exec_script(stop_path) < 0)
    {
        log_write(
            "HSV",
            "SERVICE STOP HOOK FAILED DURING SHUTDOWN",
            LOG_LEVEL_WARN
        );
    }


    if (hsv_exec_script(finish_path) < 0)
    {
        log_write(
            "HSV",
            "SERVICE FINISH HOOK FAILED",
            LOG_LEVEL_WARN
        );
    }


    log_write(
        "HSV",
        "SUPERVISOR EXITED",
        LOG_LEVEL_LOG
    );


    return EXIT_SUCCESS;
}
