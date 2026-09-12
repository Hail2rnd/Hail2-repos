#include <stdio.h>

#include "../[LOG]/log.h"
#include "../[ENG]/hinit.h"
#include "../[SYSLOAD]/sysdown.h"


int main(void)
{
    if (log_init() < 0)
    {
        fprintf(
            stderr,
            "[MD0] Failed to initialize logger\n"
        );

        return 1;
    }


    log_write(
        "MD0",
        "STARTING SHUTDOWN",
        LOG_LEVEL_LOG
    );


    log_write(
        "MD0",
        "STOPPING HSV",
        LOG_LEVEL_LOG
    );


    hsv_shutdown_all();


    log_write(
        "MD0",
        "HSV STOPPED",
        LOG_LEVEL_LOG
    );


    log_write(
        "MD0",
        "RUNNING SYSDOWN",
        LOG_LEVEL_LOG
    );


    sysdown_prepare();


    log_write(
        "MD0",
        "SYSDOWN RETURNED",
        LOG_LEVEL_LOG
    );


    log_write(
        "MD0",
        "SHUTDOWN COMPLETE",
        LOG_LEVEL_LOG
    );


    return 0;
}
