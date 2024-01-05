#include "assist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

int open_client_fd(char *host, char *port)
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == client_fd)
        return -1;

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));

    uint32_t sin_addr = (uint32_t)atol(host);
    uint16_t sin_port = (uint16_t)atoi(port);

    serv.sin_port = sin_port;
    serv.sin_family = AF_INET;
    serv.sin_addr = (struct in_addr){.s_addr = sin_addr};

    if (-1 == connect(client_fd, (SA *)&serv, sizeof(serv)))
        return -1;

    return client_fd;
}

int open_listen_fd(char *port)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd < 0)
        return -1;

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));

    uint32_t sin_addr = (uint32_t)atol("127.0.0.1");
    uint16_t sin_port = (uint16_t)atoi(port);

    serv.sin_port = sin_port;
    serv.sin_family = AF_INET;
    serv.sin_addr = (struct in_addr){.s_addr = sin_addr};

    if (-1 == bind(listen_fd, (SA *)&serv, sizeof(serv)))
        return -1;

    if (-1 == listen(listen_fd, 1024))
        return -1;

    return listen_fd;
}

void echo(int client_fd)
{
    char buf[MAXLINE];
    int nread;

    while ((nread = read(client_fd, buf, MAXLINE - 1)) > 0) {
        buf[nread] = 0;
        fprintf(stderr, "from client: %s", buf);

        /* write back to client */
        write(client_fd, buf, nread);
    }
}

void init_pool(int listen_fd, pool *pool)
{
    pool->maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++)
        pool->clientfd[i] = -1;

    pool->maxfd = listen_fd;
    FD_ZERO(&pool->read_set);
    FD_SET(listen_fd, &pool->read_set);
}

void add_client(int fd, pool *pool)
{
    pool->nready--;
    for (int i = 0; i < FD_SETSIZE; i++)
        if (pool->clientfd[i] < 0) {
            /* add client fd to the pool */
            pool->clientfd[i] = fd;
            FD_SET(fd, &pool->read_set);

            if (fd > pool->maxfd)
                pool->maxfd = fd;

            if (i > pool->maxi)
                pool->maxi = i;

            break;
        }
}

void check_clients(pool *pool)
{
    int client_fd, n;
    char buf[MAXLINE];

    for (int i = 0; (i <= pool->maxi) && (pool->nready > 0); i++) {
        client_fd = pool->clientfd[i];

        if (client_fd > 0 && FD_ISSET(client_fd, &pool->ready_set)) {
            pool->nready--;

            if ((n = read(client_fd, buf, MAXLINE - 1)) > 0) {
                /* write back to client */
                write(client_fd, buf, n);
                /* print msg into screen */
                buf[n] = 0;
                fprintf(stderr, "from client: %s", buf);

            } else { /* EOF or Errors */
                close(client_fd);
                FD_CLR(client_fd, &pool->read_set);
                pool->clientfd[i] = -1;
            }
        }
    }
}