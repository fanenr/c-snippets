#ifndef ASSIST_H
#define ASSIST_H

#define MAXLINE (64)

// on success, return value is a file descriptor, on failure, -1 is returned
extern int open_client_fd (char *host, char *port);

// on success, return value is a file descriptor, on failure, -1 is returned
extern int open_listen_fd (char *port);

// start an echo server
extern void echo (int client_fd);

#endif
