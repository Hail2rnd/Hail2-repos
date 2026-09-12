#ifndef HINIT_H
#define HINIT_H

#include <sys/types.h>


/*
 * HInit machine modes.
 *
 * MD0 = shutdown
 * MD1 = startup
 * MD2 = operating
 */
#define HINIT_MD0 0
#define HINIT_MD1 1
#define HINIT_MD2 2


/* Modes */

void mode_init(void);

void mode_start(int mode);

void mode_child_exit(
    pid_t pid,
    int status
);

int mode_get(void);


/* Load */

void load_onboot(void);

void hsv_shutdown_all(void);

void load_child_exit(
    pid_t pid,
    int status
);

int load_start_service(
    const char *service
);

int load_stop_service(
    const char *service
);

int load_restart_service(
    const char *service
);

int load_enable_service(
    const char *service
);

int load_disable_service(
    const char *service
);


/* Signals */

void signal_setup(void);

void signal_process(void);


#endif
