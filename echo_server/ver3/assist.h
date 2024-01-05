#ifndef ASSIST_H
#define ASSIST_H
#include <sys/select.h>

struct sockaddr;
typedef struct sockaddr SA;

#define MAXLINE (64)

/* on success, return value is a file descriptor, on failure, -1 is returned */
extern int open_client_fd(char *host, char *port);

/* on success, return value is a file descriptor, on failure, -1 is returned */
extern int open_listen_fd(char *port);

/* start an echo server */
extern void echo(int client_fd);

typedef struct {
    int maxi;                 /* high water index into client array */
    int maxfd;                /* largest fd in read_set */
    int nready;               /* number of ready fd from select */
    fd_set read_set;          /* set of all active fds */
    fd_set ready_set;         /* subset of fds ready for reading */
    int clientfd[FD_SETSIZE]; /* set of active fds */
} pool;

extern void init_pool(int listen_fd, pool *pool);

extern void add_client(int fd, pool *pool);

extern void check_clients(pool *pool);

#endif
