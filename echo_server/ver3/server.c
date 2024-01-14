#include "assist.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int
main (int argc, char **argv)
{
  int listen_fd, client_fd;
  socklen_t client_len;
  static pool pool;

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

  init_pool (listen_fd, &pool);
  for (;;)
    {
      pool.ready_set = pool.read_set;
      pool.nready = select (pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

      if (FD_ISSET (listen_fd, &pool.ready_set))
        {
          client_len = sizeof (client_addr);
          client_fd = accept (listen_fd, (SA *)&client_addr, &client_len);
          fprintf (stderr, "new connection\n");
          add_client (client_fd, &pool);
        }

      check_clients (&pool);
    }
}