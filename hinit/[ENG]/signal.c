#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <sys/types.h>
#include <signal.h>

#include "hinit.h"
#include "process.h"


static volatile sig_atomic_t sigchld_pending = 0;
static volatile sig_atomic_t sigterm_pending = 0;
static volatile sig_atomic_t sigint_pending = 0;


/*
 * Signal handler.
 *
 * This function must remain minimal.
 *
 * No fork().
 * No waitpid().
 * No printf().
 * No mode changes.
 * No service handling.
 *
 * It only records that an event happened.
 */
static void signal_handler(int sig)
{
    switch (sig)
    {
        case SIGCHLD:
            sigchld_pending = 1;
            break;


        case SIGTERM:
            sigterm_pending = 1;
            break;


        case SIGINT:
            sigint_pending = 1;
            break;


        default:
            break;
    }
}


/*
 * Configure HInit signal handling.
 */
void signal_setup(void)
{
    struct sigaction sa;


    sigemptyset(&sa.sa_mask);


    sa.sa_handler = signal_handler;


    sa.sa_flags =
        SA_RESTART |
        SA_NOCLDSTOP;


    if (sigaction(
            SIGCHLD,
            &sa,
            NULL
        ) < 0)
    {
        return;
    }


    if (sigaction(
            SIGTERM,
            &sa,
            NULL
        ) < 0)
    {
        return;
    }


    if (sigaction(
            SIGINT,
            &sa,
            NULL
        ) < 0)
    {
        return;
    }
}


/*
 * Process pending signals and child exits.
 *
 * This function always runs in normal process
 * context, never inside a signal handler.
 */
void signal_process(void)
{
    int status;
    pid_t pid;


    /*
     * SIGTERM requests normal system shutdown.
     *
     * mode_start(MD0) owns the complete transition:
     *
     *     MD2 -> stop MD2 -> MD0
     */
    if (sigterm_pending)
    {
        sigterm_pending = 0;

        mode_start(0);
    }


    /*
     * SIGINT currently does not perform an
     * independent action.
     *
     * It is consumed here so it does not remain
     * pending forever.
     */
    if (sigint_pending)
    {
        sigint_pending = 0;
    }


    /*
     * Collect children belonging to HInit.
     *
     * process_reap() uses WNOHANG, so this loop
     * never blocks waiting for another child.
     */
    if (sigchld_pending)
    {
        sigchld_pending = 0;


        for (;;)
        {
            pid = process_reap(&status);


            if (pid <= 0)
            {
                break;
            }


            /*
             * Each subsystem receives the event and
             * ignores PIDs that do not belong to it.
             */
            mode_child_exit(
                pid,
                status
            );


            load_child_exit(
                pid,
                status
            );
        }
    }
}
