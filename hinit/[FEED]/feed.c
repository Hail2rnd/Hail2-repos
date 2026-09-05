#include "feed.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <limits.h>


#define FEED_DIR "/run/hinit"
#define FEED_SOCKET "/run/hinit/feed.sock"


static int feed_fd = -1;


int feed_prepare(void)
{
    struct sockaddr_un addr;


    printf("hinit: preparing feed socket\n");


    mkdir(FEED_DIR, 0755);


    unlink(FEED_SOCKET);


    feed_fd = socket(AF_UNIX,
                     SOCK_STREAM,
                     0);


    if (feed_fd < 0)
    {
        perror("fail: socket");
        return -1;
    }


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
        return -1;
    }


    if (listen(feed_fd, 5) < 0)
    {
        perror("hinit: feed listen");
        close(feed_fd);
        return -1;
    }


    printf("hinit: feed ready\n");


    return 0;
}



void feed_loop(void)
{
    while (1)
    {
        int client;


        client = accept(feed_fd,
                        NULL,
                        NULL);


        if (client < 0)
        {
            perror("hinit: feed accept");
            continue;
        }


        printf("hinit: feed connection received\n");


        close(client);
    }
}
