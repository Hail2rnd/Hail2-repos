#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>

#include "../[LOG]/log.h"

#include "hinit.h"


int main(void)
{
    sigset_t block_mask;
    sigset_t old_mask;


    /*
     * Initialize Logger.
     */
    if (log_init() < 0)
    {
        /*
         * Logger itself is essential for HInit's
         * diagnostics, but HInit cannot report this
         * through Logger if Logger failed to open.
         */
        fprintf(
            stderr,
            "hinit: failed to initialize logger\n"
        );

        return 1;
    }


    log_write(
        "HINIT",
        "STARTING PID 1",
        LOG_LEVEL_LOG
    );


    signal_setup();


    sigemptyset(&block_mask);

    sigaddset(&block_mask, SIGCHLD);
    sigaddset(&block_mask, SIGTERM);
    sigaddset(&block_mask, SIGINT);


    if (sigprocmask(
            SIG_BLOCK,
            &block_mask,
            &old_mask
        ) < 0)
    {
        log_write(
            "HINIT",
            "SIGPROCMASK FAILED",
            LOG_LEVEL_CRITICAL
        );

        log_close();

        return 1;
    }


    mode_init();


    for (;;)
    {
        signal_process();

        sigsuspend(&old_mask);
    }


    return 0;
}
