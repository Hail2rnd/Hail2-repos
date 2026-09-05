#ifndef HINIT_H
#define HINIT_H


/* SYSLOAD */

void sysload_prepare(void);



/* Modes */

void mode_start(int mode);
int mode_get(void);


/* Load */

void load_onboot(void);
void load_onshut(void);
void hsv_shutdown_all(void);

/* Signals */

void signal_setup(void);


#endif
