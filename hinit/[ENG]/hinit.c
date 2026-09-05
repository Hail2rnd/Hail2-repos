#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "hinit.h"
#include "process.h"
#include "../[SYSLOAD]/sysdown.h"
#include "../[SYSLOAD]/sysload.h"

static volatile sig_atomic_t shutting_down = 0;

static void hinit_signal(int sig)
{
    if (sig == SIGTERM)
    {
        shutting_down = 1;
    }
}

int main(void)
{
    printf("hinit: starting PID 1\n");


    /*
     * Prepare the system.
     *
     * This must happen before
     * entering any machine state.
     */
    sysload_prepare();

    signal(SIGTERM, hinit_signal);

    signal_setup();


    /*
     * Inicia o modo inicial.
     * MD1 = startup
     */
    mode_start(1);



    while (!shutting_down)
    {
        pause();
    }

    return 0;
}
