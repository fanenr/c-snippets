#include "assist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main (int argc, char **argv)
{

  int client_fd;
  char *host, *port, buf[MAXLINE];

  if (argc != 3)
    {
      fprintf (stderr, "usage: %s <host> <port>\n", argv[0]);
      exit (0);
    }

  host = argv[1];
  port = argv[2];

  client_fd = open_client_fd (host, port);
  if (client_fd == -1)
    {
      fprintf (stderr, "cannot connect to %s:%s\n", host, port);
      exit (1);
    }

  int nread;
  while (fgets (buf, MAXLINE, stdin) != NULL)
    {
      write (client_fd, buf, strlen (buf));

      if ((nread = read (client_fd, buf, MAXLINE - 1)) < 1)
        break;
      buf[nread] = 0;

      fprintf (stderr, "from server: %s", buf);
    }

  close (client_fd);
}