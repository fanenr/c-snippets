#ifndef ASSIST_H
#define ASSIST_H

#include <semaphore.h>
#include <sys/select.h>

struct sockaddr;
typedef struct sockaddr SA;

#define THREADS 4
#define BUFSIZE 16
#define MAXLINE 64

/* on success, return value is a file descriptor, on failure, -1 is returned */
extern int open_client_fd (char *restrict host, char *restrict port);

/* on success, return value is a file descriptor, on failure, -1 is returned */
extern int open_listen_fd (char *port);

/* start an echo server */
extern void echo (int client_fd);

typedef struct
{
  int maxi;                 /* high water index into client array */
  int maxfd;                /* largest fd in read_set */
  int nready;               /* number of ready fd from select */
  fd_set read_set;          /* set of all active fds */
  fd_set ready_set;         /* subset of fds ready for reading */
  int clientfd[FD_SETSIZE]; /* set of active fds */
} pool;

extern void init_pool (int listen_fd, pool *pool);

extern void add_client (int fd, pool *pool);

extern void check_clients (pool *pool);

extern void *thread (void *argp);

typedef struct
{
  int *buf;    /* buffer array */
  int n;       /* maximum number of slots */
  int front;   /* buf[front + 1] is first item */
  int rear;    /* buf[rear % n] is last item*/
  sem_t mutex; /* protects accesses to buf */
  sem_t slots; /* counts available slots */
  sem_t items; /* counts available items */
} sbuf_t;

extern void sbuf_init (sbuf_t *sp, int n);

extern void sbuf_deinit (sbuf_t *sp);

extern void sbuf_insert (sbuf_t *sp, int item);

extern int sbuf_remove (sbuf_t *sp);

extern void echo_cnt (int client_fd);

extern void *thread2 (void *argp);

#endif
