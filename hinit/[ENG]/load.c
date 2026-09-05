#include "hinit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "process.h"


#define HSV_PATH "/usr/lib/hinit/hsv"
#define MAX_HSV 128

static pid_t hsv_pids[MAX_HSV];
static int hsv_count = 0;

static void load_directory(const char *directory)
{
    DIR *dir;
    struct dirent *entry;

    char link_path[PATH_MAX];
    char service_path[PATH_MAX];


    dir = opendir(directory);

    if (!dir)
    {
        perror(directory);
        return;
    }


    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, ".."))
        {
            continue;
        }


        snprintf(link_path,
                 sizeof(link_path),
                 "%s/%s",
                 directory,
                 entry->d_name);


        if (realpath(link_path, service_path) == NULL)
        {
            perror(link_path);
            continue;
        }


        pid_t pid = fork();


        if (pid < 0)
        {
            perror("hinit: fork");
            continue;
        }

        if (pid == 0)
        {
            execl(HSV_PATH,
                  HSV_PATH,
                  service_path,
                  (char *)NULL);
        
        
            perror("hinit: hsv exec");
            _exit(EXIT_FAILURE);
        }
        else
        {
            if (hsv_count < MAX_HSV)
            {
                hsv_pids[hsv_count] = pid;
                hsv_count++;
            }
        }

    }


    closedir(dir);
}



void load_onboot(void)
{
    load_directory("/etc/hinit/[LOAD]/onboot");
}



void load_onshut(void)
{
    load_directory("/etc/hinit/[LOAD]/onshut");
}

void hsv_shutdown_all(void)
{
    printf("hinit: HSV count = %d\n", hsv_count);

    for (int i = 0; i < hsv_count; i++)
    {
        printf("hinit: killing hsv %d\n", hsv_pids[i]);
        kill(hsv_pids[i], SIGTERM);
    }


    for (int i = 0; i < hsv_count; i++)
    {
        printf("hinit: waiting hsv %d\n", hsv_pids[i]);
        waitpid(hsv_pids[i], NULL, 0);
    }

    printf("hinit: all hsv stopped\n");
}
