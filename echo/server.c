#include "assist.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

sbuf_t sbuf;

int
main (int argc, char **argv)
{
  pthread_t tid;
  socklen_t client_len;
  int listen_fd, client_fd;
  struct sockaddr_storage client_addr;
  char client_hostname[MAXLINE], client_port[MAXLINE];

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s <port>\n", argv[0]);
      exit (0);
    }

  listen_fd = open_listen_fd (argv[1]);
  if (listen_fd == -1)
    {
      fprintf (stderr, "cannot create a listen socket\n");
      exit (1);
    }

  sbuf_init (&sbuf, BUFSIZE);
  for (int i = 0; i < THREADS; i++)
    pthread_create (&tid, NULL, thread, &sbuf);

  for (;;)
    {
      client_len = sizeof (client_addr);
      client_fd = accept (listen_fd, (SA *)&client_addr, &client_len);
      sbuf_insert (&sbuf, client_fd);
      fprintf (stderr, "new connection\n");
    }

  close (listen_fd);
}
