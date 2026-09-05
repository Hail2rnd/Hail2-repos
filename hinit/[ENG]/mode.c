#include "hinit.h"
#include "../[SYSLOAD]/sysdown.h"
#include <stdio.h>


static int current_mode = -1;

void mode_start(int mode)
{
    current_mode = mode;


    switch (mode)
    {
        case 1:
            printf("hinit: entering MD1\n");

            load_onboot();

            break;

        case 0:
            printf("hinit: entering MD0\n");
        
            printf("hinit: stopping hsv\n");
            hsv_shutdown_all();
        
            printf("hinit: hsv stopped\n");
        
            load_onshut();
        
            printf("hinit: running sysdown\n");
        
            sysdown_prepare();
        
            break;

        default:
            printf("hinit: unknown mode %d\n",
                   mode);

            break;
    }
}



int mode_get(void)
{
    return current_mode;
}
