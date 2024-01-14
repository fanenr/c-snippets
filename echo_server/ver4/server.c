#include "assist.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int
main (int argc, char **argv)
{
  int listen_fd, *client_fd;
  socklen_t client_len;

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

  pthread_t tid;
  for (;;)
    {
      client_len = sizeof (client_addr);
      client_fd = malloc (sizeof (int));
      *client_fd = accept (listen_fd, (SA *)&client_addr, &client_len);
      pthread_create (&tid, NULL, thread, client_fd);
    }
}