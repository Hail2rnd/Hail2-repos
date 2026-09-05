#include <stdio.h>
#include <signal.h>
#include "hinit.h"
#include "process.h"


static void signal_handler(int sig)
{
    switch (sig)
    {
        case SIGCHLD:
            process_reap();
            break;


        case SIGTERM:
            mode_start(0);
            break;

        case SIGINT:
            /*
             * Futuro:
             * tratamento de interrupção
             */
            break;


        default:
            break;
    }
}



void signal_setup(void)
{
    struct sigaction sa;


    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;


    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}
