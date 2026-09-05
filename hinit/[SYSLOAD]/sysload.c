#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <string.h>

#include "sysload.h"


/*
 * External programs
 */

#define FSCK_PATH    "/sbin/fsck"
#define MOUNT_PATH   "/bin/mount"
#define SWAPON_PATH  "/sbin/swapon"


/*
 * Internal phases
 */

static void sysload_virtualfs(void);
static int sysload_devices(void);
static void sysload_runtime(void);
static void sysload_fsck(void);
static void sysload_mounts(void);
static void sysload_swap(void);

/*
 * Internal helpers
 */

static void sysload_log(const char *message);

static int sysload_mkdir(
    const char *path,
    mode_t mode
);

static int sysload_exec(
    const char *program,
    char *const argv[]
);


/*
 * SYSLOAD entry point
 */

void sysload_prepare(void)
{
    sysload_log("Starting system preparation");

    sysload_virtualfs();

    sysload_devices();
    
    sysload_runtime();

    sysload_fsck();

    sysload_mounts();

    sysload_swap();

    sysload_log("System preparation complete");
}


/*
 * Logging
 */

static void sysload_log(const char *message)
{
    fprintf(
        stderr,
        "[SYSLOAD] %s\n",
        message
    );
}


/*
 * Directory creation helper
 */

static int sysload_mkdir(
    const char *path,
    mode_t mode
)
{
    if (mkdir(path, mode) < 0)
    {
        if (errno == EEXIST)
        {
            return 0;
        }

        fprintf(
            stderr,
            "[SYSLOAD] mkdir %s failed: %s\n",
            path,
            strerror(errno)
        );

        return -1;
    }

    return 0;
}


/*
 * Execute external programs
 */

static int sysload_exec(
    const char *program,
    char *const argv[]
)
{
    pid_t pid;
    int status;


    pid = fork();

    if (pid < 0)
    {
        sysload_log("fork failed");
        return -1;
    }


    if (pid == 0)
    {
        execv(program, argv);

        fprintf(
            stderr,
            "[SYSLOAD] exec failed: %s\n",
            strerror(errno)
        );

        _exit(127);
    }


    if (waitpid(pid, &status, 0) < 0)
    {
        sysload_log("waitpid failed");
        return -1;
    }


    if (!WIFEXITED(status))
    {
        sysload_log("Process terminated abnormally");
        return -1;
    }


    return WEXITSTATUS(status);
}


/*
 * Boot phases
 */

/*
 * Phase 1
 * Virtual filesystems
 */

static void sysload_virtualfs(void)
{
    sysload_log("Virtual filesystem phase");


    /*
     * /proc
     */

    if (sysload_mkdir("/proc", 0555) < 0)
    {
        return;
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
            sysload_log("Failed to mount /proc");
        }
    }


    /*
     * /sys
     */

    if (sysload_mkdir("/sys", 0555) < 0)
    {
        return;
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
            sysload_log("Failed to mount /sys");
        }
    }


    /*
     * /run
     */

    if (sysload_mkdir("/run", 0755) < 0)
    {
        return;
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
            sysload_log("Failed to mount /run");
        }
    }


    sysload_log("Virtual filesystems ready");
}


/*
 * Phase extra
 * Device environment
 */

static int sysload_devices(void)
{
    sysload_log("Device filesystem phase");


    if (sysload_mkdir("/dev", 0755) < 0)
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
            sysload_log("Failed mounting /dev");
            return -1;
        }
    }


    if (sysload_mkdir("/dev/pts", 0755) < 0)
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
            sysload_log("Failed mounting /dev/pts");
            return -1;
        }
    }


    if (sysload_mkdir("/dev/shm", 01777) < 0)
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
            sysload_log("Failed mounting /dev/shm");
            return -1;
        }
    }


    sysload_log("Device filesystem ready");

    return 0;
}


/*
 * Phase 2
 * Runtime environment
 */

static void sysload_runtime(void)
{
    sysload_log("Runtime phase");


    if (sysload_mkdir(
            "/run/hinit",
            0755
        ) < 0)
    {
        sysload_log("Failed creating /run/hinit");
    }


    if (sysload_mkdir(
            "/run/lock",
            0775
        ) < 0)
    {
        sysload_log("Failed creating /run/lock");
    }


    if (sysload_mkdir(
            "/run/user",
            0755
        ) < 0)
    {
        sysload_log("Failed creating /run/user");
    }


    sysload_log("Runtime environment ready");
}


/*
 * Phase 3
 * Filesystem check
 */

static void sysload_fsck(void)
{
    sysload_log("Filesystem check phase");


    int result;

    char *argv[] =
    {
        FSCK_PATH,
        "-A",
        "-V",
        NULL
    };


    result = sysload_exec(
        FSCK_PATH,
        argv
    );


    if (result < 0)
    {
        sysload_log("Failed to execute fsck");
        return;
    }


    if (result == 0)
    {
        sysload_log("Filesystem check completed");
        return;
    }


    sysload_log("Filesystem check returned warnings");
}


/*
 * Phase 4
 * Local filesystems
 */

static void sysload_mounts(void)
{
    sysload_log("Mounting local filesystems");


    int result;

    char *argv[] =
    {
        MOUNT_PATH,
        "-a",
        NULL
    };


    result = sysload_exec(
        MOUNT_PATH,
        argv
    );


    if (result < 0)
    {
        sysload_log("Failed to execute mount");
        return;
    }


    if (result != 0)
    {
        sysload_log("Mount returned warnings");
        return;
    }


    sysload_log("Local filesystems mounted");
}


/*
 * Phase 5
 * Swap
 */

static void sysload_swap(void)
{
    sysload_log("Activating swap");


    int result;

    char *argv[] =
    {
        SWAPON_PATH,
        "-a",
        NULL
    };


    result = sysload_exec(
        SWAPON_PATH,
        argv
    );


    if (result < 0)
    {
        sysload_log("Failed to execute swapon");
        return;
    }


    if (result != 0)
    {
        sysload_log("Swap activation returned warnings");
        return;
    }


    sysload_log("Swap activated");
}
