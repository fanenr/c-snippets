#include "assist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    
    int client_fd;
    char *host, *port, buf[MAXLINE] = {0};

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    client_fd = open_client_fd(host, port);
    if (client_fd == -1) {
        fprintf(stderr, "cannot connect to %s:%s\n", host, port);
        exit(1);
    }

    while (fgets(buf, MAXLINE, stdin) != NULL) {
        write(client_fd, buf, MAXLINE);
        memset(buf, 0, MAXLINE);

        if (read(client_fd, buf, MAXLINE) < 1)
            break;

        fprintf(stderr, "from server: ");
        write(2, buf, MAXLINE);
        memset(buf, 0, MAXLINE);
    }

    close(client_fd);
}