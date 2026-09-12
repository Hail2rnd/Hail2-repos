#include "hinit.h"
#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#include "../[LOG]/log.h"


#define HSV_PATH "/usr/lib/hinit/hsv"
#define MAX_HSV 128

#define SERVICE_DIR "/etc/hinit/[SV]"
#define ONBOOT_DIR  "/etc/hinit/[LOAD]/onboot"


struct hsv_entry
{
    pid_t pid;
    char service[PATH_MAX];
};


static struct hsv_entry hsv_table[MAX_HSV];
static size_t hsv_count = 0;


/*
 * Build the service directory path.
 */
static int load_service_path(
    char *buffer,
    size_t buffer_size,
    const char *service
)
{
    int length;


    if (buffer == NULL ||
        buffer_size == 0 ||
        service == NULL ||
        service[0] == '\0')
    {
        return -1;
    }


    /*
     * Service names are names, not arbitrary paths.
     */
    if (strchr(service, '/') != NULL)
    {
        return -1;
    }


    length = snprintf(
        buffer,
        buffer_size,
        "%s/%s",
        SERVICE_DIR,
        service
    );


    if (length < 0 ||
        (size_t)length >= buffer_size)
    {
        return -1;
    }


    return 0;
}


/*
 * Find an HSV by its service path.
 */
static ssize_t hsv_find(
    const char *service_path
)
{
    size_t i;


    if (service_path == NULL)
    {
        return -1;
    }


    for (i = 0; i < hsv_count; i++)
    {
        if (strcmp(
                hsv_table[i].service,
                service_path
            ) == 0)
        {
            return (ssize_t)i;
        }
    }


    return -1;
}


/*
 * Register one HSV process.
 */
static int hsv_register(
    pid_t pid,
    const char *service_path
)
{
    if (pid <= 0 ||
        service_path == NULL)
    {
        return -1;
    }


    if (hsv_count >= MAX_HSV)
    {
        return -1;
    }


    if (strlen(service_path) >= PATH_MAX)
    {
        return -1;
    }


    if (hsv_find(service_path) >= 0)
    {
        return -1;
    }


    hsv_table[hsv_count].pid = pid;


    strcpy(
        hsv_table[hsv_count].service,
        service_path
    );


    hsv_count++;


    return 0;
}


/*
 * Remove one HSV entry.
 */
static void hsv_unregister(
    size_t index
)
{
    if (index >= hsv_count)
    {
        return;
    }


    if (index + 1 < hsv_count)
    {
        memmove(
            &hsv_table[index],
            &hsv_table[index + 1],
            (hsv_count - index - 1) *
                sizeof(struct hsv_entry)
        );
    }


    hsv_count--;
}


/*
 * Notify the loader that a child exited.
 *
 * Only HSV children are handled here.
 */
void load_child_exit(
    pid_t pid,
    int status
)
{
    size_t i;


    for (i = 0; i < hsv_count; i++)
    {
        if (hsv_table[i].pid == pid)
        {
            hsv_unregister(i);

            (void)status;

            return;
        }
    }


    (void)status;
}


/*
 * Start one HSV for a service.
 */
int load_start_service(
    const char *service
)
{
    char service_path[PATH_MAX];
    char message[256];

    struct stat st;
    pid_t pid;


    if (load_service_path(
            service_path,
            sizeof(service_path),
            service
        ) < 0)
    {
        log_write(
            "LOAD",
            "INVALID SERVICE NAME",
            LOG_LEVEL_WARN
        );

        return -1;
    }


    if (stat(
            service_path,
            &st
        ) < 0)
    {
        snprintf(
            message,
            sizeof(message),
            "SERVICE %s DOES NOT EXIST: %s",
            service,
            strerror(errno)
        );

        log_write(
            "LOAD",
            message,
            LOG_LEVEL_WARN
        );

        return -1;
    }


    if (!S_ISDIR(st.st_mode))
    {
        snprintf(
            message,
            sizeof(message),
            "SERVICE %s IS NOT A DIRECTORY",
            service
        );

        log_write(
            "LOAD",
            message,
            LOG_LEVEL_WARN
        );

        return -1;
    }


    /*
     * Already running.
     */
    if (hsv_find(service_path) >= 0)
    {
        return 0;
    }


    if (hsv_count >= MAX_HSV)
    {
        log_write(
            "LOAD",
            "MAXIMUM HSV LIMIT REACHED",
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    /*
     * Protect the fork/register window from SIGCHLD.
     */
    {
        sigset_t set;
        sigset_t oldset;


        sigemptyset(&set);


        sigaddset(
            &set,
            SIGCHLD
        );


        if (sigprocmask(
                SIG_BLOCK,
                &set,
                &oldset
            ) < 0)
        {
            log_write(
                "LOAD",
                "SIGPROCMASK FAILED",
                LOG_LEVEL_CRITICAL
            );

            return -1;
        }


        pid = fork();


        if (pid < 0)
        {
            log_write(
                "LOAD",
                "FORK FAILED WHILE STARTING HSV",
                LOG_LEVEL_CRITICAL
            );


            sigprocmask(
                SIG_SETMASK,
                &oldset,
                NULL
            );


            return -1;
        }


        /*
         * HSV child.
         */
        if (pid == 0)
        {
            sigprocmask(
                SIG_SETMASK,
                &oldset,
                NULL
            );


            execl(
                HSV_PATH,
                HSV_PATH,
                service_path,
                (char *)NULL
            );


            /*
             * Logger is not used here because this is
             * the HSV child after fork and before exec.
             */
            dprintf(
                STDERR_FILENO,
                "hinit: hsv exec failed: %s\n",
                strerror(errno)
            );


            _exit(EXIT_FAILURE);
        }


        /*
         * Register before SIGCHLD can be processed.
         */
        if (hsv_register(
                pid,
                service_path
            ) < 0)
        {
            snprintf(
                message,
                sizeof(message),
                "FAILED TO REGISTER HSV FOR %s",
                service
            );

            log_write(
                "LOAD",
                message,
                LOG_LEVEL_CRITICAL
            );


            if (kill(
                    pid,
                    SIGTERM
                ) < 0 &&
                errno != ESRCH)
            {
                log_write(
                    "LOAD",
                    "FAILED TO TERMINATE UNREGISTERED HSV",
                    LOG_LEVEL_CRITICAL
                );
            }


            process_wait(pid);


            sigprocmask(
                SIG_SETMASK,
                &oldset,
                NULL
            );


            return -1;
        }


        sigprocmask(
            SIG_SETMASK,
            &oldset,
            NULL
        );
    }


    snprintf(
        message,
        sizeof(message),
        "STARTED SERVICE %s (HSV %ld)",
        service,
        (long)pid
    );


    log_write(
        "LOAD",
        message,
        LOG_LEVEL_LOG
    );


    return 0;
}


/*
 * Stop one HSV.
 *
 * The HSV itself performs the service shutdown.
 */
int load_stop_service(
    const char *service
)
{
    char service_path[PATH_MAX];
    char message[256];

    ssize_t index;
    pid_t pid;


    if (load_service_path(
            service_path,
            sizeof(service_path),
            service
        ) < 0)
    {
        return -1;
    }


    index = hsv_find(service_path);


    if (index < 0)
    {
        /*
         * Service is already stopped.
         */
        return 0;
    }


    pid = hsv_table[index].pid;


    if (kill(
            pid,
            SIGTERM
        ) < 0)
    {
        if (errno == ESRCH)
        {
            /*
             * The HSV already exited.
             * SIGCHLD will normally clean the table.
             */
            return 0;
        }


        snprintf(
            message,
            sizeof(message),
            "FAILED TO STOP SERVICE %s: %s",
            service,
            strerror(errno)
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    snprintf(
        message,
        sizeof(message),
        "REQUESTED STOP FOR SERVICE %s",
        service
    );


    log_write(
        "LOAD",
        message,
        LOG_LEVEL_LOG
    );


    return 0;
}


/*
 * Restart one service.
 *
 * HSV receives SIGUSR1 and performs the actual
 * service restart.
 */
int load_restart_service(
    const char *service
)
{
    char service_path[PATH_MAX];
    char message[256];

    ssize_t index;
    pid_t pid;


    if (load_service_path(
            service_path,
            sizeof(service_path),
            service
        ) < 0)
    {
        return -1;
    }


    index = hsv_find(service_path);


    if (index < 0)
    {
        /*
         * There is no HSV to restart.
         */
        return -1;
    }


    pid = hsv_table[index].pid;


    if (kill(
            pid,
            SIGUSR1
        ) < 0)
    {
        if (errno == ESRCH)
        {
            return -1;
        }


        snprintf(
            message,
            sizeof(message),
            "FAILED TO RESTART SERVICE %s: %s",
            service,
            strerror(errno)
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    snprintf(
        message,
        sizeof(message),
        "REQUESTED RESTART FOR SERVICE %s",
        service
    );


    log_write(
        "LOAD",
        message,
        LOG_LEVEL_LOG
    );


    return 0;
}


/*
 * Enable one service for startup.
 *
 * /etc/hinit/[LOAD]/onboot/<service>
 * ->
 * /etc/hinit/[SV]/<service>
 */
int load_enable_service(
    const char *service
)
{
    char service_path[PATH_MAX];
    char link_path[PATH_MAX];
    char message[256];

    struct stat st;


    if (load_service_path(
            service_path,
            sizeof(service_path),
            service
        ) < 0)
    {
        return -1;
    }


    if (stat(
            service_path,
            &st
        ) < 0)
    {
        snprintf(
            message,
            sizeof(message),
            "SERVICE %s DOES NOT EXIST: %s",
            service,
            strerror(errno)
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_WARN
        );

        return -1;
    }


    if (!S_ISDIR(st.st_mode))
    {
        snprintf(
            message,
            sizeof(message),
            "SERVICE %s IS NOT A DIRECTORY",
            service
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_WARN
        );

        return -1;
    }


    if (snprintf(
            link_path,
            sizeof(link_path),
            "%s/%s",
            ONBOOT_DIR,
            service
        ) >= (int)sizeof(link_path))
    {
        log_write(
            "LOAD",
            "ONBOOT LINK PATH TOO LONG",
            LOG_LEVEL_WARN
        );

        return -1;
    }


    /*
     * Replace an existing link cleanly.
     */
    if (unlink(link_path) < 0 &&
        errno != ENOENT)
    {
        snprintf(
            message,
            sizeof(message),
            "CANNOT REPLACE ENABLE LINK FOR %s: %s",
            service,
            strerror(errno)
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    if (symlink(
            service_path,
            link_path
        ) < 0)
    {
        snprintf(
            message,
            sizeof(message),
            "CANNOT ENABLE SERVICE %s: %s",
            service,
            strerror(errno)
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    snprintf(
        message,
        sizeof(message),
        "ENABLED SERVICE %s",
        service
    );


    log_write(
        "LOAD",
        message,
        LOG_LEVEL_LOG
    );


    return 0;
}


/*
 * Disable one service from startup.
 */
int load_disable_service(
    const char *service
)
{
    char link_path[PATH_MAX];
    char message[256];


    if (service == NULL ||
        service[0] == '\0' ||
        strchr(service, '/') != NULL)
    {
        log_write(
            "LOAD",
            "INVALID SERVICE NAME FOR DISABLE",
            LOG_LEVEL_WARN
        );

        return -1;
    }


    if (snprintf(
            link_path,
            sizeof(link_path),
            "%s/%s",
            ONBOOT_DIR,
            service
        ) >= (int)sizeof(link_path))
    {
        log_write(
            "LOAD",
            "ONBOOT DISABLE LINK PATH TOO LONG",
            LOG_LEVEL_WARN
        );

        return -1;
    }


    if (unlink(link_path) < 0)
    {
        if (errno == ENOENT)
        {
            return 0;
        }


        snprintf(
            message,
            sizeof(message),
            "CANNOT DISABLE SERVICE %s: %s",
            service,
            strerror(errno)
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    snprintf(
        message,
        sizeof(message),
        "DISABLED SERVICE %s",
        service
    );


    log_write(
        "LOAD",
        message,
        LOG_LEVEL_LOG
    );


    return 0;
}


/*
 * Load all services represented by symlinks
 * inside the [LOAD] directory.
 */
static int load_directory(
    const char *directory
)
{
    DIR *dir;
    struct dirent *entry;

    char link_path[PATH_MAX];
    char service_path[PATH_MAX];
    char message[256];

    int result = 0;


    if (directory == NULL)
    {
        return -1;
    }


    dir = opendir(directory);


    if (dir == NULL)
    {
        snprintf(
            message,
            sizeof(message),
            "CANNOT OPEN LOAD DIRECTORY %s: %s",
            directory,
            strerror(errno)
        );


        log_write(
            "LOAD",
            message,
            LOG_LEVEL_CRITICAL
        );

        return -1;
    }


    while ((entry = readdir(dir)) != NULL)
    {
        struct stat link_stat;
        struct stat service_stat;


        if (!strcmp(
                entry->d_name,
                "."
            ) ||
            !strcmp(
                entry->d_name,
                ".."
            ))
        {
            continue;
        }


        if (snprintf(
                link_path,
                sizeof(link_path),
                "%s/%s",
                directory,
                entry->d_name
            ) >= (int)sizeof(link_path))
        {
            snprintf(
                message,
                sizeof(message),
                "SERVICE PATH TOO LONG: %s",
                entry->d_name
            );


            log_write(
                "LOAD",
                message,
                LOG_LEVEL_WARN
            );


            result = -1;
            continue;
        }


        if (lstat(
                link_path,
                &link_stat
            ) < 0)
        {
            snprintf(
                message,
                sizeof(message),
                "LSTAT FAILED FOR %s: %s",
                link_path,
                strerror(errno)
            );


            log_write(
                "LOAD",
                message,
                LOG_LEVEL_CRITICAL
            );


            result = -1;
            continue;
        }


        if (!S_ISLNK(
                link_stat.st_mode
            ))
        {
            snprintf(
                message,
                sizeof(message),
                "IGNORING NON-SYMLINK SERVICE ENTRY: %s",
                link_path
            );


            log_write(
                "LOAD",
                message,
                LOG_LEVEL_WARN
            );


            result = -1;
            continue;
        }


        if (realpath(
                link_path,
                service_path
            ) == NULL)
        {
            snprintf(
                message,
                sizeof(message),
                "REALPATH FAILED FOR %s: %s",
                link_path,
                strerror(errno)
            );


            log_write(
                "LOAD",
                message,
                LOG_LEVEL_WARN
            );


            result = -1;
            continue;
        }


        if (stat(
                service_path,
                &service_stat
            ) < 0)
        {
            snprintf(
                message,
                sizeof(message),
                "SERVICE TARGET STAT FAILED FOR %s: %s",
                service_path,
                strerror(errno)
            );


            log_write(
                "LOAD",
                message,
                LOG_LEVEL_WARN
            );


            result = -1;
            continue;
        }


        if (!S_ISDIR(
                service_stat.st_mode
            ))
        {
            snprintf(
                message,
                sizeof(message),
                "SERVICE TARGET IS NOT A DIRECTORY: %s",
                service_path
            );


            log_write(
                "LOAD",
                message,
                LOG_LEVEL_WARN
            );


            result = -1;
            continue;
        }


        /*
         * Start the HSV through the normal
         * single-service entry point.
         */
        if (load_start_service(
                entry->d_name
            ) < 0)
        {
            result = -1;
        }
    }


    closedir(dir);


    return result;
}


/*
 * Start services enabled for startup.
 */
void load_onboot(void)
{
    load_directory(
        ONBOOT_DIR
    );
}


/*
 * Stop every HSV currently owned by HInit.
 */
void hsv_shutdown_all(void)
{
    char message[128];


    snprintf(
        message,
        sizeof(message),
        "HSV COUNT = %zu",
        hsv_count
    );


    log_write(
        "LOAD",
        message,
        LOG_LEVEL_LOG
    );


    /*
     * Request every HSV to shut down.
     */
    while (hsv_count > 0)
    {
        pid_t pid;
        int status;


        pid = hsv_table[
            hsv_count - 1
        ].pid;


        if (kill(
                pid,
                SIGTERM
            ) < 0)
        {
            if (errno != ESRCH)
            {
                log_write(
                    "LOAD",
                    "FAILED TO TERMINATE HSV DURING SHUTDOWN",
                    LOG_LEVEL_CRITICAL
                );
            }
        }


        /*
         * Wait for this exact HSV.
         */
        status = process_wait(pid);


        if (status < 0)
        {
            log_write(
                "LOAD",
                "FAILED TO WAIT FOR HSV DURING SHUTDOWN",
                LOG_LEVEL_CRITICAL
            );


            hsv_unregister(
                hsv_count - 1
            );

            continue;
        }


        hsv_unregister(
            hsv_count - 1
        );
    }


    log_write(
        "LOAD",
        "ALL HSV STOPPED",
        LOG_LEVEL_LOG
    );
}
