#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "sysload.h"


/*
 * External programs.
 */
#define FSCK_PATH    "/sbin/fsck"
#define MOUNT_PATH   "/bin/mount"
#define SWAPON_PATH  "/sbin/swapon"


/*
 * Internal phases.
 */
static int sysload_virtualfs(void);
static int sysload_devices(void);
static int sysload_runtime(void);

static void sysload_fsck(void);
static void sysload_mounts(void);
static void sysload_swap(void);


/*
 * Internal helpers.
 */
static void sysload_log(
    const char *message
);

static int sysload_mkdir(
    const char *path,
    mode_t mode
);

static int sysload_exec(
    const char *program,
    char *const argv[]
);


/*
 * SYSLOAD entry point.
 *
 * The startup environment is divided into two
 * classes:
 *
 * Essential:
 *
 *     virtual filesystems
 *     device filesystems
 *     HInit runtime
 *
 * Non-essential / recoverable:
 *
 *     fsck result
 *     mount -a result
 *     swapon -a result
 *
 * The latter are reported as warnings and do
 * not automatically abort startup.
 */
void sysload_prepare(void)
{
    int result;


    sysload_log(
        "Starting system preparation"
    );


    /*
     * Essential phase:
     * virtual filesystems.
     */
    result = sysload_virtualfs();

    if (result < 0)
    {
        sysload_log(
            "Virtual filesystem phase failed"
        );

        /*
         * Do not continue pretending that the
         * runtime environment is completely ready.
         */
        return;
    }


    /*
     * Essential phase:
     * device filesystem environment.
     */
    result = sysload_devices();

    if (result < 0)
    {
        sysload_log(
            "Device filesystem phase failed"
        );

        return;
    }


    /*
     * Essential phase:
     * HInit runtime environment.
     */
    result = sysload_runtime();

    if (result < 0)
    {
        sysload_log(
            "Runtime environment phase failed"
        );

        return;
    }


    /*
     * Recoverable phases.
     *
     * Their failures are logged as warnings
     * inside the respective functions.
     */
    sysload_fsck();

    sysload_mounts();

    sysload_swap();


    sysload_log(
        "System preparation complete"
    );
}


/*
 * SYSLOAD logging.
 *
 * The future Logger layer can replace this
 * implementation without changing the phases.
 */
static void sysload_log(
    const char *message
)
{
    if (message == NULL)
    {
        return;
    }


    fprintf(
        stderr,
        "[SYSLOAD] %s\n",
        message
    );
}


/*
 * Create a directory if necessary.
 *
 * EEXIST is only considered success when the
 * existing path is actually a directory.
 */
static int sysload_mkdir(
    const char *path,
    mode_t mode
)
{
    struct stat st;


    if (path == NULL)
    {
        return -1;
    }


    if (mkdir(
            path,
            mode
        ) == 0)
    {
        return 0;
    }


    if (errno != EEXIST)
    {
        fprintf(
            stderr,
            "[SYSLOAD] mkdir %s failed: %s\n",
            path,
            strerror(errno)
        );

        return -1;
    }


    /*
     * Something already exists at this path.
     *
     * It must be a directory.
     */
    if (stat(
            path,
            &st
        ) < 0)
    {
        fprintf(
            stderr,
            "[SYSLOAD] stat %s failed: %s\n",
            path,
            strerror(errno)
        );

        return -1;
    }


    if (!S_ISDIR(st.st_mode))
    {
        fprintf(
            stderr,
            "[SYSLOAD] %s exists but is not a directory\n",
            path
        );

        return -1;
    }


    return 0;
}


/*
 * Execute one external program synchronously.
 *
 * Returns:
 *
 *     >= 0  -> normal exit status
 *     -1    -> execution/wait failure
 */
static int sysload_exec(
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
            "[SYSLOAD] fork %s failed: %s\n",
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
            "[SYSLOAD] exec %s failed: %s\n",
            program,
            strerror(errno)
        );


        _exit(127);
    }


    /*
     * Wait only for the child created by this
     * SYSLOAD operation.
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
            "[SYSLOAD] waitpid %s failed: %s\n",
            program,
            strerror(errno)
        );

        return -1;
    }


    /*
     * A signal termination is not a normal exit.
     */
    if (WIFSIGNALED(status))
    {
        fprintf(
            stderr,
            "[SYSLOAD] %s terminated by signal %d\n",
            program,
            WTERMSIG(status)
        );

        return -1;
    }


    if (!WIFEXITED(status))
    {
        fprintf(
            stderr,
            "[SYSLOAD] %s terminated abnormally\n",
            program
        );

        return -1;
    }


    return WEXITSTATUS(status);
}


/*
 * Phase 1
 *
 * Virtual filesystems.
 *
 * Essential for the operating environment.
 */
static int sysload_virtualfs(void)
{
    sysload_log(
        "Virtual filesystem phase"
    );


    /*
     * /proc
     */
    if (sysload_mkdir(
            "/proc",
            0555
        ) < 0)
    {
        return -1;
    }


    if (mount(
            "proc",
            "/proc",
            "proc",
            0,
            NULL
        ) < 0)
    {
        if (errno != EBUSY)
        {
            fprintf(
                stderr,
                "[SYSLOAD] Failed to mount /proc: %s\n",
                strerror(errno)
            );

            return -1;
        }
    }


    /*
     * /sys
     */
    if (sysload_mkdir(
            "/sys",
            0555
        ) < 0)
    {
        return -1;
    }


    if (mount(
            "sysfs",
            "/sys",
            "sysfs",
            0,
            NULL
        ) < 0)
    {
        if (errno != EBUSY)
        {
            fprintf(
                stderr,
                "[SYSLOAD] Failed to mount /sys: %s\n",
                strerror(errno)
            );

            return -1;
        }
    }


    /*
     * /run
     */
    if (sysload_mkdir(
            "/run",
            0755
        ) < 0)
    {
        return -1;
    }


    if (mount(
            "tmpfs",
            "/run",
            "tmpfs",
            0,
            "mode=0755"
        ) < 0)
    {
        if (errno != EBUSY)
        {
            fprintf(
                stderr,
                "[SYSLOAD] Failed to mount /run: %s\n",
                strerror(errno)
            );

            return -1;
        }
    }


    sysload_log(
        "Virtual filesystems ready"
    );


    return 0;
}


/*
 * Phase 2
 *
 * Device filesystem environment.
 *
 * Essential for normal userspace operation.
 */
static int sysload_devices(void)
{
    sysload_log(
        "Device filesystem phase"
    );


    /*
     * /dev
     */
    if (sysload_mkdir(
            "/dev",
            0755
        ) < 0)
    {
        return -1;
    }


    if (mount(
            "devtmpfs",
            "/dev",
            "devtmpfs",
            0,
            NULL
        ) < 0)
    {
        if (errno != EBUSY)
        {
            fprintf(
                stderr,
                "[SYSLOAD] Failed to mount /dev: %s\n",
                strerror(errno)
            );

            return -1;
        }
    }


    /*
     * /dev/pts
     */
    if (sysload_mkdir(
            "/dev/pts",
            0755
        ) < 0)
    {
        return -1;
    }


    if (mount(
            "devpts",
            "/dev/pts",
            "devpts",
            0,
            NULL
        ) < 0)
    {
        if (errno != EBUSY)
        {
            fprintf(
                stderr,
                "[SYSLOAD] Failed to mount /dev/pts: %s\n",
                strerror(errno)
            );

            return -1;
        }
    }


    /*
     * /dev/shm
     */
    if (sysload_mkdir(
            "/dev/shm",
            01777
        ) < 0)
    {
        return -1;
    }


    if (mount(
            "tmpfs",
            "/dev/shm",
            "tmpfs",
            0,
            "mode=1777"
        ) < 0)
    {
        if (errno != EBUSY)
        {
            fprintf(
                stderr,
                "[SYSLOAD] Failed to mount /dev/shm: %s\n",
                strerror(errno)
            );

            return -1;
        }
    }


    sysload_log(
        "Device filesystem ready"
    );


    return 0;
}


/*
 * Phase 3
 *
 * HInit runtime environment.
 *
 * Essential because HInit's own components use
 * /run/hinit.
 */
static int sysload_runtime(void)
{
    sysload_log(
        "Runtime phase"
    );


    if (sysload_mkdir(
            "/run/hinit",
            0755
        ) < 0)
    {
        return -1;
    }


    if (sysload_mkdir(
            "/run/lock",
            0775
        ) < 0)
    {
        return -1;
    }


    if (sysload_mkdir(
            "/run/user",
            0755
        ) < 0)
    {
        return -1;
    }


    sysload_log(
        "Runtime environment ready"
    );


    return 0;
}


/*
 * Phase 4
 *
 * Filesystem check.
 *
 * fsck returning non-zero does not automatically
 * mean that HInit itself must fail.
 */
static void sysload_fsck(void)
{
    int result;

    char *argv[] =
    {
        FSCK_PATH,
        "-A",
        "-V",
        NULL
    };


    sysload_log(
        "Filesystem check phase"
    );


    result = sysload_exec(
        FSCK_PATH,
        argv
    );


    if (result < 0)
    {
        sysload_log(
            "Filesystem check execution failed WARN"
        );

        return;
    }


    if (result == 0)
    {
        sysload_log(
            "Filesystem check completed"
        );

        return;
    }


    /*
     * fsck has its own exit-status semantics.
     *
     * For now SYSLOAD reports a warning and lets
     * the remaining startup phases continue.
     */
    fprintf(
        stderr,
        "[SYSLOAD] Filesystem check returned status %d WARN\n",
        result
    );
}


/*
 * Phase 5
 *
 * Local filesystems.
 *
 * mount -a failure is reported, but is not
 * automatically converted into HInit failure.
 */
static void sysload_mounts(void)
{
    int result;

    char *argv[] =
    {
        MOUNT_PATH,
        "-a",
        NULL
    };


    sysload_log(
        "Mounting local filesystems"
    );


    result = sysload_exec(
        MOUNT_PATH,
        argv
    );


    if (result < 0)
    {
        sysload_log(
            "Mount execution failed WARN"
        );

        return;
    }


    if (result != 0)
    {
        fprintf(
            stderr,
            "[SYSLOAD] mount -a returned status %d WARN\n",
            result
        );

        return;
    }


    sysload_log(
        "Local filesystems mounted"
    );
}


/*
 * Phase 6
 *
 * Swap.
 *
 * Swap failure is recoverable; the system can
 * continue operating without swap.
 */
static void sysload_swap(void)
{
    int result;

    char *argv[] =
    {
        SWAPON_PATH,
        "-a",
        NULL
    };


    sysload_log(
        "Activating swap"
    );


    result = sysload_exec(
        SWAPON_PATH,
        argv
    );


    if (result < 0)
    {
        sysload_log(
            "Swap activation execution failed WARN"
        );

        return;
    }


    if (result != 0)
    {
        fprintf(
            stderr,
            "[SYSLOAD] swapon -a returned status %d WARN\n",
            result
        );

        return;
    }


    sysload_log(
        "Swap activated"
    );
}
