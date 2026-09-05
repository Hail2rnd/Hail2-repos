#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/reboot.h>

#include "sysdown.h"

int sysdown_prepare(void)
{
    printf("[SYSDOWN] Starting system shutdown preparation\n");

    printf("[SYSDOWN] Deactivating swap\n");
    system("swapoff -a");

    printf("[SYSDOWN] Unmounting local filesystems\n");
    system("umount -a");

    printf("[SYSDOWN] Doing sync\n");
    sync();

    printf("[SYSDOWN] System shutdown preparation complete\n");

    printf("[SYSDOWN] Powering off\n");

    if (reboot(RB_POWER_OFF) < 0)
    {
        perror("[SYSDOWN] reboot");
    }

    return 0;
}
