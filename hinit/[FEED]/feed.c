#define _POSIX_C_SOURCE 200809L

#include "feed.h"

#include "../[ENG]/hinit.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>


#define FEED_DIR    "/run/hinit"
#define FEED_SOCKET "/run/hinit/feed.sock"

#define FEED_BACKLOG 8
#define FEED_BUFFER  256


static int feed_fd = -1;


static int feed_set_nonblock(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0) {
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }

    return 0;
}


static int feed_valid_service(const char *service)
{
    const unsigned char *p;

    if (service == NULL || service[0] == '\0') {
        return 0;
    }

    p = (const unsigned char *)service;

    while (*p != '\0')
    {
        if (*p == '/' ||
            isspace(*p) ||
            *p == '\\')
        {
            return 0;
        }

        p++;
    }

    return 1;
}


static void feed_reply(int client, const char *message)
{
    if (message == NULL) {
        return;
    }

    (void)send(client,
               message,
               strlen(message),
               MSG_NOSIGNAL);
}


static void feed_command(int client, char *command)
{
    char *cmd;
    char *arg;

    cmd = strtok(command, " \t\r\n");
    arg = strtok(NULL, " \t\r\n");

    if (cmd == NULL) {
        feed_reply(client, "ERR empty command\n");
        return;
    }


    /*
     * setmode <mode>
     *
     * Mode ownership remains in MODE.
     */
    if (strcmp(cmd, "setmode") == 0)
    {
        char *end;
        long mode;

        if (arg == NULL) {
            feed_reply(client, "ERR missing mode\n");
            return;
        }

        mode = strtol(arg, &end, 10);

        if (*end != '\0' ||
            mode < HINIT_MD0 ||
            mode > HINIT_MD2)
        {
            feed_reply(client, "ERR invalid mode\n");
            return;
        }

        mode_start((int)mode);

        feed_reply(client, "OK\n");
        return;
    }


    /*
     * All service operations require a service name.
     */
    if (arg == NULL)
    {
        feed_reply(client, "ERR missing service\n");
        return;
    }

    if (!feed_valid_service(arg))
    {
        feed_reply(client, "ERR invalid service\n");
        return;
    }


    /*
     * Service enable/disable belongs to LOAD.
     */
    if (strcmp(cmd, "enable") == 0)
    {
        if (load_enable_service(arg) < 0) {
            feed_reply(client, "ERR enable failed\n");
            return;
        }

        feed_reply(client, "OK\n");
        return;
    }


    if (strcmp(cmd, "disable") == 0)
    {
        if (load_disable_service(arg) < 0) {
            feed_reply(client, "ERR disable failed\n");
            return;
        }

        feed_reply(client, "OK\n");
        return;
    }


    /*
     * Service lifecycle belongs to HSV through LOAD's
     * service registry/control functions.
     */
    if (strcmp(cmd, "start") == 0)
    {
        if (load_start_service(arg) < 0) {
            feed_reply(client, "ERR start failed\n");
            return;
        }

        feed_reply(client, "OK\n");
        return;
    }


    if (strcmp(cmd, "stop") == 0)
    {
        if (load_stop_service(arg) < 0) {
            feed_reply(client, "ERR stop failed\n");
            return;
        }

        feed_reply(client, "OK\n");
        return;
    }


    if (strcmp(cmd, "restart") == 0)
    {
        if (load_restart_service(arg) < 0) {
            feed_reply(client, "ERR restart failed\n");
            return;
        }

        feed_reply(client, "OK\n");
        return;
    }


    feed_reply(client, "ERR unknown command\n");
}


int feed_prepare(void)
{
    struct sockaddr_un addr;

    printf("hinit: preparing feed socket\n");


    if (mkdir(FEED_DIR, 0755) < 0 &&
        errno != EEXIST)
    {
        perror("hinit: feed mkdir");
        return -1;
    }


    unlink(FEED_SOCKET);


    feed_fd = socket(AF_UNIX,
                     SOCK_STREAM,
                     0);

    if (feed_fd < 0)
    {
        perror("hinit: feed socket");
        return -1;
    }


    if (feed_set_nonblock(feed_fd) < 0)
    {
        perror("hinit: feed nonblock");

        close(feed_fd);
        feed_fd = -1;

        return -1;
    }


    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;

    snprintf(addr.sun_path,
             sizeof(addr.sun_path),
             "%s",
             FEED_SOCKET);


    if (bind(feed_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0)
    {
        perror("hinit: feed bind");

        close(feed_fd);
        feed_fd = -1;

        return -1;
    }


    if (listen(feed_fd, FEED_BACKLOG) < 0)
    {
        perror("hinit: feed listen");

        close(feed_fd);
        feed_fd = -1;

        return -1;
    }


    printf("hinit: feed ready\n");

    return 0;
}


void feed_loop(void)
{
    int client;
    char buffer[FEED_BUFFER];
    ssize_t length;


    if (feed_fd < 0) {
        return;
    }


    for (;;)
    {
        client = accept(feed_fd, NULL, NULL);

        if (client < 0)
        {
            if (errno == EAGAIN ||
                errno == EWOULDBLOCK)
            {
                return;
            }

            if (errno == EINTR) {
                continue;
            }

            perror("hinit: feed accept");
            return;
        }


        if (feed_set_nonblock(client) < 0)
        {
            feed_reply(client, "ERR client setup failed\n");
            close(client);
            continue;
        }


        length = recv(client,
                      buffer,
                      sizeof(buffer) - 1,
                      0);


        if (length < 0)
        {
            if (errno != EAGAIN &&
                errno != EWOULDBLOCK &&
                errno != EINTR)
            {
                feed_reply(client, "ERR receive failed\n");
            }

            close(client);
            continue;
        }


        if (length == 0)
        {
            close(client);
            continue;
        }


        buffer[length] = '\0';

        feed_command(client, buffer);

        close(client);
    }
}
