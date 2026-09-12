#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "sysdown.h"


/*
 * External programs.
 */
#define SWAPOFF_PATH "/sbin/swapoff"
#define UMOUNT_PATH  "/bin/umount"


/*
 * Execute one shutdown program synchronously.
 *
 * Returns:
 *
 *     >= 0  -> normal exit status
 *     -1    -> execution/wait failure
 */
static int sysdown_exec(
    const char *program,
    char *const argv[]
)
{
    pid_t pid;
    int status;


    if (program == NULL ||
        argv == NULL)
    {
        return -1;
    }


    pid = fork();


    if (pid < 0)
    {
        fprintf(
            stderr,
            "[SYSDOWN] fork %s failed: %s\n",
            program,
            strerror(errno)
        );

        return -1;
    }


    if (pid == 0)
    {
        execv(
            program,
            argv
        );


        fprintf(
            stderr,
            "[SYSDOWN] exec %s failed: %s\n",
            program,
            strerror(errno)
        );


        _exit(127);
    }


    /*
     * Wait only for the child created by this
     * shutdown operation.
     */
    for (;;)
    {
        if (waitpid(
                pid,
                &status,
                0
            ) >= 0)
        {
            break;
        }


        if (errno == EINTR)
        {
            continue;
        }


        fprintf(
            stderr,
            "[SYSDOWN] waitpid %s failed: %s\n",
            program,
            strerror(errno)
        );

        return -1;
    }


    if (WIFSIGNALED(status))
    {
        fprintf(
            stderr,
            "[SYSDOWN] %s terminated by signal %d\n",
            program,
            WTERMSIG(status)
        );

        return -1;
    }


    if (!WIFEXITED(status))
    {
        fprintf(
            stderr,
            "[SYSDOWN] %s terminated abnormally\n",
            program
        );

        return -1;
    }


    return WEXITSTATUS(status);
}


/*
 * Prepare the system for shutdown and request
 * the final poweroff.
 */
int sysdown_prepare(void)
{
    int result;


    printf(
        "[SYSDOWN] Starting system shutdown preparation\n"
    );


    /*
     * Deactivate swap.
     *
     * Failure here is reported but does not prevent
     * the remaining shutdown sequence.
     */
    printf(
        "[SYSDOWN] Deactivating swap\n"
    );


    {
        char *argv[] =
        {
            SWAPOFF_PATH,
            "-a",
            NULL
        };


        result = sysdown_exec(
            SWAPOFF_PATH,
            argv
        );
    }


    if (result < 0)
    {
        fprintf(
            stderr,
            "[SYSDOWN] swapoff execution failed WARN\n"
        );
    }
    else if (result != 0)
    {
        fprintf(
            stderr,
            "[SYSDOWN] swapoff -a returned status %d WARN\n",
            result
        );
    }


    /*
     * Unmount local filesystems.
     *
     * Some filesystems may legitimately refuse to
     * unmount during shutdown, so this remains a
     * warning rather than an HInit fatal condition.
     */
    printf(
        "[SYSDOWN] Unmounting local filesystems\n"
    );


    {
        char *argv[] =
        {
            UMOUNT_PATH,
            "-a",
            NULL
        };


        result = sysdown_exec(
            UMOUNT_PATH,
            argv
        );
    }


    if (result < 0)
    {
        fprintf(
            stderr,
            "[SYSDOWN] umount execution failed WARN\n"
        );
    }
    else if (result != 0)
    {
        fprintf(
            stderr,
            "[SYSDOWN] umount -a returned status %d WARN\n",
            result
        );
    }


    /*
     * Flush pending filesystem writes before
     * requesting poweroff.
     */
    printf(
        "[SYSDOWN] Doing sync\n"
    );


    sync();


    printf(
        "[SYSDOWN] System shutdown preparation complete\n"
    );


    /*
     * Final shutdown operation.
     *
     * On success, reboot() does not return.
     */
    printf(
        "[SYSDOWN] Powering off\n"
    );


    if (reboot(RB_POWER_OFF) < 0)
    {
        fprintf(
            stderr,
            "[SYSDOWN] reboot failed: %s\n",
            strerror(errno)
        );

        return -1;
    }


    /*
     * Normally unreachable because a successful
     * reboot() transfers control to the kernel.
     */
    return 0;
}
