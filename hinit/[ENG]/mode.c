#include "hinit.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "../[LOG]/log.h"

#include "process.h"


#define MD1_PATH "/usr/lib/hinit/md1"
#define MD2_PATH "/usr/lib/hinit/md2"
#define MD0_PATH "/usr/lib/hinit/md0"


#define MODE_NONE  (-1)
#define MODE_MD0   0
#define MODE_MD1   1
#define MODE_MD2   2


static int current_mode = MODE_NONE;


/*
 * PID of the persistent MD2 process.
 *
 * A value greater than zero means that MD2
 * is currently being tracked by HInit.
 */
static pid_t md2_pid = -1;


/*
 * Set when HInit intentionally asks MD2 to leave.
 *
 * This allows mode_child_exit() to distinguish:
 *
 *     normal MD2 termination
 *
 * from:
 *
 *     unexpected MD2 death.
 */
static int md2_shutdown_requested = 0;


/*
 * Execute a one-shot machine mode.
 *
 * Used by MD1 and MD0.
 *
 * Returns:
 *
 *     0..255     -> normal exit status
 *     -1         -> fork/wait failure
 *     -128-sig   -> process was terminated by signal
 */
static int mode_exec(const char *program)
{
    pid_t pid;
    int status;


    if (program == NULL)
    {
        log_write(
            "MODE",
            "MODE EXEC INVALID PROGRAM",
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    pid = process_spawn(program);


    if (pid < 0)
    {
        return -1;
    }


    /*
     * MD1 and MD0 are synchronous one-shot modes.
     */
    status = process_wait(pid);


    if (status < 0)
    {
        return -1;
    }


    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }


    if (WIFSIGNALED(status))
    {
        return -128 - WTERMSIG(status);
    }


    return -1;
}


/*
 * Report a mode failure.
 *
 * The severity depends on the context:
 *
 *     signal termination -> CRITICAL
 *     execution/wait     -> CRITICAL
 *     non-zero exit      -> CRITICAL
 *
 * Unexpected MD2 death is promoted to FATAL
 * by mode_child_exit().
 */
static void mode_failure(
    const char *component,
    int status
)
{
    char message[128];


    if (component == NULL)
    {
        component = "MODE";
    }


    if (status < -128)
    {
        snprintf(
            message,
            sizeof(message),
            "%s TERMINATED BY SIGNAL %d",
            component,
            -128 - status
        );

        log_write(
            "MODE",
            message,
            LOG_LEVEL_CRITICAL
        );

        return;
    }


    if (status == -1)
    {
        snprintf(
            message,
            sizeof(message),
            "%s FAILED TO EXECUTE OR WAIT",
            component
        );

        log_write(
            "MODE",
            message,
            LOG_LEVEL_CRITICAL
        );

        return;
    }


    snprintf(
        message,
        sizeof(message),
        "%s EXITED WITH STATUS %d",
        component,
        status
    );


    log_write(
        "MODE",
        message,
        LOG_LEVEL_CRITICAL
    );
}


/*
 * Stop and reap the persistent MD2 process.
 *
 * MD2 belongs to the mode controller.
 * Therefore the mode controller is responsible
 * for terminating and collecting it during the
 * MD2 -> MD0 transition.
 */
static int mode_stop_md2(void)
{
    pid_t pid;
    int status;


    pid = md2_pid;


    if (pid <= 0)
    {
        md2_pid = -1;
        return 0;
    }


    /*
     * Mark the termination as intentional before
     * sending SIGTERM.
     */
    md2_shutdown_requested = 1;


    /*
     * Ask MD2 to leave its operating loop.
     *
     * ESRCH is not immediately fatal here:
     * the process may have already exited and
     * simply be waiting to be reaped.
     */
    if (process_signal(pid, SIGTERM) < 0)
    {
        /*
         * Do not clear md2_pid yet.
         *
         * process_wait() below still owns the
         * responsibility of collecting this child.
         */
    }


    /*
     * Reap exactly the MD2 child.
     *
     * This is deliberately NOT process_reap(),
     * because process_reap() is global and could
     * consume an unrelated HInit child such as HSV.
     */
    status = process_wait(pid);


    if (status < 0)
    {
        md2_pid = -1;
        md2_shutdown_requested = 0;

        mode_failure(
            "MD2",
            -1
        );

        return -1;
    }


    /*
     * MD2 has now definitely left the process table.
     */
    md2_pid = -1;


    /*
     * Consume the intentional MD2 exit here.
     *
     * mode_child_exit() is normally called by the
     * signal layer for asynchronous child exits.
     * During shutdown we already waited for MD2
     * directly, so there is no SIGCHLD event left
     * for the normal reaper to own.
     */
    md2_shutdown_requested = 0;


    if (WIFEXITED(status))
    {
        if (WEXITSTATUS(status) != 0)
        {
            mode_failure(
                "MD2",
                WEXITSTATUS(status)
            );

            return -1;
        }


        log_write(
            "MODE",
            "MD2 EXITED DURING SHUTDOWN",
            LOG_LEVEL_LOG
        );


        return 0;
    }


    if (WIFSIGNALED(status))
    {
        /*
         * SIGTERM is the expected shutdown signal.
         */
        if (WTERMSIG(status) == SIGTERM)
        {
            log_write(
                "MODE",
                "MD2 EXITED DURING SHUTDOWN",
                LOG_LEVEL_LOG
            );

            return 0;
        }


        mode_failure(
            "MD2",
            -128 - WTERMSIG(status)
        );

        return -1;
    }


    mode_failure(
        "MD2",
        -1
    );

    return -1;
}


/*
 * Initialize machine modes.
 */
void mode_init(void)
{
    log_write(
        "MODE",
        "INITIALIZING MACHINE MODES",
        LOG_LEVEL_LOG
    );


    mode_start(MODE_MD1);
}


/*
 * Start/change machine mode.
 */
void mode_start(int mode)
{
    switch (mode)
    {
        /*
         * ---------------------------------------------------------
         * MD1 - STARTUP
         * ---------------------------------------------------------
         */
        case MODE_MD1:

            if (current_mode == MODE_MD1)
            {
                log_write(
                    "MODE",
                    "ALREADY IN MD1",
                    LOG_LEVEL_WARN
                );

                return;
            }


            if (current_mode == MODE_MD2)
            {
                log_write(
                    "MODE",
                    "INVALID MODE TRANSITION MD2 -> MD1",
                    LOG_LEVEL_WARN
                );

                return;
            }


            if (current_mode == MODE_MD0)
            {
                log_write(
                    "MODE",
                    "INVALID MODE TRANSITION MD0 -> MD1",
                    LOG_LEVEL_WARN
                );

                return;
            }


            log_write(
                "MODE",
                "ENTERING MD1",
                LOG_LEVEL_LOG
            );


            current_mode = MODE_MD1;


            {
                int status;


                status = mode_exec(MD1_PATH);


                if (status != 0)
                {
                    mode_failure(
                        "MD1",
                        status
                    );

                    current_mode = MODE_NONE;

                    return;
                }
            }


            log_write(
                "MODE",
                "MD1 COMPLETE",
                LOG_LEVEL_LOG
            );


            /*
             * Successful startup automatically
             * enters the operating state.
             */
            mode_start(MODE_MD2);

            break;


        /*
         * ---------------------------------------------------------
         * MD2 - OPERATING
         * ---------------------------------------------------------
         */
        case MODE_MD2:

            if (current_mode == MODE_MD2)
            {
                log_write(
                    "MODE",
                    "ALREADY IN MD2",
                    LOG_LEVEL_WARN
                );

                return;
            }


            if (current_mode == MODE_MD0)
            {
                log_write(
                    "MODE",
                    "INVALID MODE TRANSITION MD0 -> MD2",
                    LOG_LEVEL_WARN
                );

                return;
            }


            if (current_mode != MODE_MD1)
            {
                char message[128];


                snprintf(
                    message,
                    sizeof(message),
                    "INVALID MODE TRANSITION MD%d -> MD2",
                    current_mode
                );


                log_write(
                    "MODE",
                    message,
                    LOG_LEVEL_WARN
                );

                return;
            }


            log_write(
                "MODE",
                "ENTERING MD2",
                LOG_LEVEL_LOG
            );


            md2_shutdown_requested = 0;


            md2_pid = process_spawn(MD2_PATH);


            if (md2_pid < 0)
            {
                mode_failure(
                    "MD2",
                    -1
                );

                md2_pid = -1;
                current_mode = MODE_NONE;

                return;
            }


            /*
             * Only publish MD2 as the current state
             * after the child was successfully created
             * and is being tracked.
             */
            current_mode = MODE_MD2;


            {
                char message[128];


                snprintf(
                    message,
                    sizeof(message),
                    "MD2 OPERATING PID = %ld",
                    (long)md2_pid
                );


                log_write(
                    "MODE",
                    message,
                    LOG_LEVEL_LOG
                );
            }

            break;


        /*
         * ---------------------------------------------------------
         * MD0 - SHUTDOWN
         * ---------------------------------------------------------
         */
        case MODE_MD0:

            if (current_mode == MODE_MD0)
            {
                log_write(
                    "MODE",
                    "ALREADY IN MD0",
                    LOG_LEVEL_WARN
                );

                return;
            }


            if (current_mode != MODE_MD2)
            {
                char message[128];


                snprintf(
                    message,
                    sizeof(message),
                    "INVALID MODE TRANSITION MD%d -> MD0",
                    current_mode
                );


                log_write(
                    "MODE",
                    message,
                    LOG_LEVEL_WARN
                );

                return;
            }


            log_write(
                "MODE",
                "ENTERING MD0",
                LOG_LEVEL_LOG
            );


            /*
             * MD2 must leave first.
             *
             * The mode controller owns MD2, so it
             * terminates and reaps it before starting
             * the one-shot shutdown program.
             */
            if (mode_stop_md2() < 0)
            {
                /*
                 * MD2 failed to shut down cleanly.
                 *
                 * Continue with MD0 anyway: the machine
                 * is already in the shutdown transition
                 * and MD0 owns the remaining shutdown work.
                 */
                log_write(
                    "MODE",
                    "CONTINUING SHUTDOWN AFTER MD2 SHUTDOWN FAILURE",
                    LOG_LEVEL_CRITICAL
                );
            }


            current_mode = MODE_MD0;


            {
                int status;


                status = mode_exec(MD0_PATH);


                if (status != 0)
                {
                    mode_failure(
                        "MD0",
                        status
                    );

                    return;
                }
            }


            log_write(
                "MODE",
                "MD0 COMPLETE",
                LOG_LEVEL_LOG
            );

            break;


        /*
         * ---------------------------------------------------------
         * INVALID MODE
         * ---------------------------------------------------------
         */
        default:
        {
            char message[128];


            snprintf(
                message,
                sizeof(message),
                "UNKNOWN MODE %d",
                mode
            );


            log_write(
                "MODE",
                message,
                LOG_LEVEL_WARN
            );

            return;
        }
    }
}


/*
 * Notify the mode controller that one of its
 * tracked children has exited.
 *
 * This function is deliberately outside a signal
 * handler. The signal layer can collect the child
 * event and normal HInit control flow can call
 * this function safely.
 */
void mode_child_exit(
    pid_t pid,
    int status
)
{
    /*
     * We currently only have one persistent mode
     * process: MD2.
     */
    if (pid != md2_pid)
    {
        return;
    }


    /*
     * MD2 is no longer alive.
     */
    md2_pid = -1;


    /*
     * If shutdown was explicitly requested,
     * MD2 leaving is normal.
     */
    if (md2_shutdown_requested)
    {
        md2_shutdown_requested = 0;

        log_write(
            "MODE",
            "MD2 EXITED DURING SHUTDOWN",
            LOG_LEVEL_LOG
        );

        return;
    }


    /*
     * MD2 died while the machine was operating.
     *
     * This is an actual operating-state failure.
     */
    if (current_mode == MODE_MD2)
    {
        char message[128];


        snprintf(
            message,
            sizeof(message),
            "MD2 PROCESS EXITED UNEXPECTEDLY (STATUS %d)",
            status
        );


        log_write(
            "MODE",
            message,
            LOG_LEVEL_FATAL
        );


        current_mode = MODE_NONE;
    }
}


/*
 * Return the current machine mode.
 */
int mode_get(void)
{
    return current_mode;
}
