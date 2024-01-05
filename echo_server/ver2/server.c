#include "assist.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>

void sigchld_handler(int sig)
{
    while (waitpid(-1, 0, WNOHANG) > 0)
        ;
}

int main(int argc, char **argv)
{
    int listen_fd, client_fd;
    socklen_t client_len;

    struct sockaddr_storage client_addr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    signal(SIGCHLD, sigchld_handler);
    listen_fd = open_listen_fd(argv[1]);
    if (listen_fd == -1) {
        fprintf(stderr, "cannot create a listen socket\n");
        exit(1);
    }

    for (;;) {
        client_len = sizeof(client_addr);
        client_fd = accept(listen_fd, (SA)&client_addr, &client_len);

        if (fork() == 0) {
            close(listen_fd);
            echo(client_fd);
            close(client_fd);
            exit(0);
        }

        fprintf(stderr, "new connection\n");
        close(client_fd);
    }
}