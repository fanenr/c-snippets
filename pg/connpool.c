#include "connpool.h"

#include <stddef.h>
#include <stdlib.h>

#define container_of(ptr, type, member)                                       \
  ((type *) ((void *) (ptr) - offsetof (type, member)))

void
connpool_free (connpool_t *pool)
{
  connpool_conn_t *conn, *next;
  pthread_cond_destroy (&pool->cond);
  pthread_mutex_destroy (&pool->mtx);

  for (conn = pool->head; conn; conn = next)
    {
      PQfinish (conn->conn);
      next = conn->next;
      free (conn);
    }
}

connpool_conn_t *
connpool_acquire (connpool_t *pool)
{
  connpool_conn_t *conn;

  pthread_mutex_lock (&pool->mtx);
  for (; !(conn = pool->head);)
    pthread_cond_wait (&pool->cond, &pool->mtx);

  if (!(pool->head = conn->next))
    pool->tail = NULL;
  pthread_mutex_unlock (&pool->mtx);

  conn->next = NULL;
  return conn;
}

void
connpool_release (connpool_t *pool, connpool_conn_t *c)
{
  connpool_conn_t *tail;

  pthread_mutex_lock (&pool->mtx);
  if ((tail = pool->tail))
    pool->tail = tail->next = c;
  else
    pool->head = pool->tail = c;
  pthread_mutex_unlock (&pool->mtx);

  pthread_cond_signal (&pool->cond);
}

static inline int
init_conn (connpool_t *pool)
{
  connpool_conn_t *conn, *tail;

  if (!(conn = malloc (sizeof (connpool_conn_t))))
    return CONNPOOL_ERR_MALLOC;

  conn->next = NULL;
  conn->conn = PQconnectdb (pool->info);

  if (PQstatus (conn->conn) != CONNECTION_OK)
    return (free (conn), CONNPOOL_ERR_CONNECT);

  if ((tail = pool->tail))
    pool->tail = tail->next = conn;
  else
    pool->head = pool->tail = conn;

  return 0;
}

int
connpool_init (connpool_t *pool, const char *info, size_t n)
{
  *pool = (connpool_t) { .info = info };

  if (0 != pthread_mutex_init (&pool->mtx, NULL))
    return CONNPOOL_ERR_MTX;
  if (0 != pthread_cond_init (&pool->cond, NULL))
    return CONNPOOL_ERR_COND;

  int init_conn_ret;
  n = n ? n : CONNPOOL_DEFAULT;

  for (size_t i = 0; i < n; i++)
    if ((init_conn_ret = init_conn (pool)))
      return (connpool_free (pool), init_conn_ret);

  pool->size = n;
  return 0;
}
