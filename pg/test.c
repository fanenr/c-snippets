#include "connpool.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define HOST "localhost"
#define PORT "5432"

#define DBNAME "postgres"
#define USER "arthur"
#define PASS "12345"

const char *conn_info
    = "postgresql://" USER ":" PASS "@" HOST ":" PORT "/" DBNAME;

#define N 16

void *
work (void *arg)
{
  connpool_conn_t *c;
  connpool_t *pool = arg;

  if (!(c = connpool_acquire (pool)))
    abort ();

  if (PQstatus (c->conn) != CONNECTION_OK)
    {
      printf ("conn err: %s\n", PQerrorMessage (c->conn));
      PQreset (c->conn);
      abort ();
    }

  struct timespec tm = {
    .tv_nsec = (1 + rand () % 500) * 1000 * 1000,
  };

  for (; nanosleep (&tm, &tm) == -1 && errno == EINTR;)
    ;

  connpool_release (pool, c);

  return NULL;
}

int
main (void)
{
  connpool_t pool;
  pthread_t threads[N];

  srand (time (NULL));

  if (connpool_init (&pool, conn_info, N) != 0)
    abort ();

  for (int i = 0; i < N; i++)
    pthread_create (&threads[i], NULL, work, &pool);

  for (int i = 0; i < N; i++)
    pthread_join (threads[i], NULL);

  connpool_free (&pool);
}
