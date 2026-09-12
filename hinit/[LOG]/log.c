#define _POSIX_C_SOURCE 200809L

#include "log.h"

#include <stdio.h>
#include <time.h>


static FILE *log_file = NULL;


static const char *log_level_name(
    int level
)
{
    switch (level)
    {
        case LOG_LEVEL_LOG:
            return "LOG";

        case LOG_LEVEL_WARN:
            return "WARN";

        case LOG_LEVEL_CRITICAL:
            return "CRITICAL";

        case LOG_LEVEL_FATAL:
            return "FATAL";

        default:
            return "UNKNOWN";
    }
}


int log_init(void)
{
    if (log_file != NULL)
    {
        return 0;
    }


    log_file = fopen(
        LOG_FILE,
        "a"
    );


    if (log_file == NULL)
    {
        return -1;
    }


    /*
     * Write each log line immediately.
     */
    setvbuf(
        log_file,
        NULL,
        _IOLBF,
        0
    );


    return 0;
}


int log_write(
    const char *component,
    const char *message,
    int level
)
{
    time_t now;
    struct tm tm_now;


    if (log_file == NULL)
    {
        return -1;
    }


    if (component == NULL ||
        message == NULL)
    {
        return -1;
    }


    /*
     * Get the current system time.
     */
    now = time(NULL);


    if (now == (time_t)-1)
    {
        return -1;
    }


    /*
     * Convert to local system time.
     */
    if (localtime_r(
            &now,
            &tm_now
        ) == NULL)
    {
        return -1;
    }


    /*
     * Final format:
     *
     * [MM/DD/YYYY] [HH:MM] COMPONENT MESSAGE LEVEL
     *
     * Seconds are intentionally not included.
     */
    if (fprintf(
            log_file,
            "[%02d/%02d/%04d] [%02d:%02d] %s %s %s\n",
            tm_now.tm_mon + 1,
            tm_now.tm_mday,
            tm_now.tm_year + 1900,
            tm_now.tm_hour,
            tm_now.tm_min,
            component,
            message,
            log_level_name(level)
        ) < 0)
    {
        return -1;
    }


    fflush(log_file);


    return 0;
}


void log_close(void)
{
    if (log_file == NULL)
    {
        return;
    }


    fclose(log_file);

    log_file = NULL;
}
