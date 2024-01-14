#include "assist.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

void command ();

int
main (int argc, char **argv)
{
  int listen_fd, client_fd;
  socklen_t client_len;

  struct sockaddr_storage client_addr;
  char client_hostname[MAXLINE], client_port[MAXLINE];

  fd_set read_set, ready_set;

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

  FD_ZERO (&read_set);
  FD_SET (STDIN_FILENO, &read_set);
  FD_SET (listen_fd, &read_set);

  for (;;)
    {
      ready_set = read_set;
      select (listen_fd + 1, &ready_set, NULL, NULL, NULL);

      if (FD_ISSET (STDIN_FILENO, &ready_set))
        command ();

      if (FD_ISSET (listen_fd, &ready_set))
        {
          fprintf (stderr, "new connection\n");
          client_len = sizeof (client_addr);
          client_fd = accept (listen_fd, (SA *)&client_addr, &client_len);
          echo (client_fd);
        }

      close (client_fd);
    }
}

void
command ()
{
  char buf[MAXLINE];
  if (!fgets (buf, MAXLINE, stdin))
    exit (0);
  fprintf (stderr, "from console: %s", buf);
}