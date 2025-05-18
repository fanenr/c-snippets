#include "assist.h"

#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
open_client_fd (const char *host, const char *port)
{
  int sock;
  struct sockaddr_in serv = {};
  uint32_t sin_addr = atol (host);
  uint16_t sin_port = atoi (port);

  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    return -1;

  serv.sin_port = sin_port;
  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = sin_addr;

  if (connect (sock, (SA *) &serv, sizeof (serv)) == -1)
    {
      close (sock);
      return -1;
    }

  return sock;
}

int
open_listen_fd (const char *port)
{
  int sock = socket (AF_INET, SOCK_STREAM, 0);

  if (sock < 0)
    return -1;

  struct sockaddr_in serv = {};
  uint32_t sin_addr = htonl (INADDR_ANY);
  uint16_t sin_port = atoi (port);

  serv.sin_port = sin_port;
  serv.sin_family = AF_INET;
  serv.sin_addr = (struct in_addr) { .s_addr = sin_addr };

  if (-1 == bind (sock, (SA *) &serv, sizeof (serv)))
    return -1;

  if (-1 == listen (sock, 1024))
    return -1;

  return sock;
}

void
echo (int client_fd)
{
  ssize_t nread;
  char buf[MAXLINE];

  while ((nread = read (client_fd, buf, MAXLINE - 1)) > 0)
    {
      buf[nread] = 0;
      fprintf (stderr, "from client: %s", buf);
      /* write back to client */
      write (client_fd, buf, nread);
    }
}

void
init_pool (int listen_fd, pool *pool)
{
  pool->maxi = -1;
  for (int i = 0; i < FD_SETSIZE; i++)
    pool->clientfd[i] = -1;

  pool->maxfd = listen_fd;
  FD_ZERO (&pool->read_set);
  FD_SET (listen_fd, &pool->read_set);
}

void
add_client (int fd, pool *pool)
{
  pool->nready--;
  for (int i = 0; i < FD_SETSIZE; i++)
    if (pool->clientfd[i] < 0)
      {
	/* add client fd to the pool */
	pool->clientfd[i] = fd;
	FD_SET (fd, &pool->read_set);

	if (fd > pool->maxfd)
	  pool->maxfd = fd;

	if (i > pool->maxi)
	  pool->maxi = i;

	break;
      }
}

void
check_clients (pool *pool)
{
  int client_fd, n;
  char buf[MAXLINE];

  for (int i = 0; (i <= pool->maxi) && (pool->nready > 0); i++)
    {
      client_fd = pool->clientfd[i];

      if (client_fd > 0 && FD_ISSET (client_fd, &pool->ready_set))
	{
	  pool->nready--;

	  if ((n = read (client_fd, buf, MAXLINE - 1)) > 0)
	    {
	      /* write back to client */
	      write (client_fd, buf, n);
	      /* print msg into screen */
	      buf[n] = 0;
	      fprintf (stderr, "from client: %s", buf);
	    }
	  else
	    { /* EOF or Errors */
	      close (client_fd);
	      FD_CLR (client_fd, &pool->read_set);
	      pool->clientfd[i] = -1;
	    }
	}
    }
}

void
sbuf_init (sbuf_t *sp, int n)
{
  sp->n = n;
  sp->front = sp->rear = 0;
  sp->buf = calloc (n, sizeof (int));

  sem_init (&sp->mutex, 0, 1);
  sem_init (&sp->slots, 0, n);
  sem_init (&sp->items, 0, 0);
}

void
sbuf_deinit (sbuf_t *sp)
{
  free (sp->buf);
}

void
sbuf_insert (sbuf_t *sp, int item)
{
  sem_wait (&sp->slots);
  sem_wait (&sp->mutex);
  sp->buf[(++sp->rear) % sp->n] = item;
  sem_post (&sp->mutex);
  sem_post (&sp->items); /* announce available item */
}

int
sbuf_remove (sbuf_t *sp)
{
  int item;
  sem_wait (&sp->items);
  sem_wait (&sp->mutex);
  item = sp->buf[(++sp->front) % sp->n];
  sem_post (&sp->mutex);
  sem_post (&sp->slots); /* announce available slot */
  return item;
}

static int byte_cnt;
static sem_t mutex;

static void
init_echo_cnt (void)
{
  sem_init (&mutex, 0, 1);
  byte_cnt = 0;
}

void
echo_cnt (int client_fd)
{
  int nread;
  char buf[MAXLINE];

  static pthread_once_t once = PTHREAD_ONCE_INIT;
  pthread_once (&once, init_echo_cnt);

  while ((nread = read (client_fd, buf, MAXLINE - 1)) > 0)
    {
      buf[nread] = 0;

      sem_wait (&mutex);
      byte_cnt += nread;
      fprintf (stderr, "from client: %s (%d / %d)", buf, nread, byte_cnt);
      sem_post (&mutex);

      /* write back to client */
      write (client_fd, buf, nread);
    }
}

void *
thread (void *sbuf)
{
  pthread_detach (pthread_self ());

  for (;;)
    {
      int fd = sbuf_remove (sbuf);
      echo_cnt (fd);
      close (fd);
    }

  return NULL;
}
