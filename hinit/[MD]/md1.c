#include <stdio.h>

#include "../[LOG]/log.h"
#include "../[SYSLOAD]/sysload.h"
#include "../[ENG]/hinit.h"


int main(void)
{
    if (log_init() < 0)
    {
        fprintf(
            stderr,
            "[MD1] Failed to initialize logger\n"
        );

        return 1;
    }


    log_write(
        "MD1",
        "STARTING SYSTEM STARTUP",
        LOG_LEVEL_LOG
    );


    log_write(
        "MD1",
        "PREPARING SYSTEM ENVIRONMENT",
        LOG_LEVEL_LOG
    );


    sysload_prepare();


    log_write(
        "MD1",
        "LOADING ONBOOT SERVICES",
        LOG_LEVEL_LOG
    );


    load_onboot();


    log_write(
        "MD1",
        "STARTUP COMPLETE",
        LOG_LEVEL_LOG
    );


    return 0;
}
