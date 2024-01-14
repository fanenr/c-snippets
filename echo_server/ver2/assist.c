#include "assist.h"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
open_client_fd (char *host, char *port)
{
  int client_fd = socket (AF_INET, SOCK_STREAM, 0);

  if (-1 == client_fd)
    return -1;

  struct sockaddr_in serv;
  memset (&serv, 0, sizeof (serv));

  uint32_t sin_addr = (uint32_t)atol (host);
  uint16_t sin_port = (uint16_t)atoi (port);

  serv.sin_port = sin_port;
  serv.sin_family = AF_INET;
  serv.sin_addr = (struct in_addr){ .s_addr = sin_addr };

  if (-1 == connect (client_fd, (SA)&serv, sizeof (serv)))
    return -1;

  return client_fd;
}

int
open_listen_fd (char *port)
{
  int listen_fd = socket (AF_INET, SOCK_STREAM, 0);

  if (listen_fd < 0)
    return -1;

  struct sockaddr_in serv;
  memset (&serv, 0, sizeof (serv));

  uint32_t sin_addr = (uint32_t)atol ("127.0.0.1");
  uint16_t sin_port = (uint16_t)atoi (port);

  serv.sin_port = sin_port;
  serv.sin_family = AF_INET;
  serv.sin_addr = (struct in_addr){ .s_addr = sin_addr };

  if (-1 == bind (listen_fd, (SA)&serv, sizeof (serv)))
    return -1;

  if (-1 == listen (listen_fd, 1024))
    return -1;

  return listen_fd;
}

void
echo (int client_fd)
{
  char buf[MAXLINE];

  while (read (client_fd, buf, MAXLINE) > 0)
    {
      fprintf (stderr, "from client: ");
      write (2, buf, MAXLINE);

      /* write back to client */
      write (client_fd, buf, MAXLINE);
    }
}