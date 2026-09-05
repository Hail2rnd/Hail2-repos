#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>

#include "process.h"

static volatile sig_atomic_t shutting_down = 0;

static void hsv_signal(int sig)
{
    if (sig == SIGTERM)
    {
        shutting_down = 1;
    }
}

static void hsv_exec_script(const char *path)
{
    pid_t pid;


    if (access(path, X_OK) != 0)
    {
        return;
    }


    pid = fork();


    if (pid < 0)
    {
        perror("hsv: fork");
        return;
    }


    if (pid == 0)
    {
        execl(
            path,
            path,
            (char *)NULL
        );


        perror("hsv: exec script");
        _exit(EXIT_FAILURE);
    }


    waitpid(pid, NULL, 0);
}

int main(int argc, char *argv[])
{
    char run_path[PATH_MAX];
    char stop_path[PATH_MAX];
    char finish_path[PATH_MAX];

    struct sigaction sa = {0};
    
    sa.sa_handler = hsv_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGTERM, &sa, NULL);

    if (argc != 2)
    {
        fprintf(stderr,
                "Usage: %s <service>\n",
                argv[0]);

        return EXIT_FAILURE;
    }


    snprintf(run_path,
             sizeof(run_path),
             "%s/run",
             argv[1]);

    snprintf(stop_path,
             sizeof(stop_path),
             "%s/stop",
             argv[1]);

    snprintf(finish_path,
             sizeof(finish_path),
             "%s/finish",
             argv[1]);
while (1)
{
    if (shutting_down)
    {
        hsv_exec_script(stop_path);

        hsv_exec_script(finish_path);

        break;
    }

    pid_t pid = process_spawn(run_path);


    if (pid < 0)
    {
        sleep(1);
        continue;
    }


    process_wait(pid);


    if (shutting_down)
    {
        break;
    }


    sleep(1);
}

    return EXIT_SUCCESS;
}
