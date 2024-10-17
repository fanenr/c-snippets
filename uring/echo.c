#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <liburing.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLK_SIZE 4096
#define QUEUE_DEPTH 64

enum
{
  EV_TIMEOUT,
  EV_ACCEPT,
  EV_RECV,
  EV_SEND,
};

union io_data
{
  __u64 u64;
  struct
  {
    int type : 3;
    intptr_t ctx : 61;
  };
};

struct context
{
  int fd;
  socklen_t addrlen;
  struct sockaddr addr;

  void *in;
  size_t inlen;

  void *out;
  size_t outlen;
};

#define ptr2ctx(ptr) ((intptr_t)ptr >> 3)
#define ctx2ptr(ctx) ((void *)((intptr_t)ctx << 3))

#define error(code, fmt, ...)                                                 \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "%s:%s:%d: error: ", __FILE__, __FUNCTION__,           \
               __LINE__);                                                     \
      fprintf (stderr, fmt, ##__VA_ARGS__);                                   \
      fprintf (stderr, "\n");                                                 \
      exit (code);                                                            \
    }                                                                         \
  while (0)

#define memalloc(size)                                                        \
  ({                                                                          \
    void *ret;                                                                \
    if (!(ret = malloc (size)))                                               \
      error (1, "%s", strerror (errno));                                      \
    ret;                                                                      \
  })

#define wait_cqe()                                                            \
  ({                                                                          \
    int ret;                                                                  \
    struct io_uring_cqe *cqe;                                                 \
    if ((ret = io_uring_wait_cqe (&ring, &cqe)))                              \
      error (1, "%s", strerror (-ret));                                       \
    cqe;                                                                      \
  })

#define submit()                                                              \
  do                                                                          \
    {                                                                         \
      int ret;                                                                \
      if ((ret = io_uring_submit (&ring)) < 0)                                \
        error (1, "%s", strerror (-ret));                                     \
    }                                                                         \
  while (0)

static void drop_conn (struct context *ctx);
static void hnd_cqe (struct io_uring_cqe *cqe);

static int serv;
static struct io_uring ring;
struct __kernel_timespec ts = { 10 };

struct io_uring_sqe *
get_sqe (void)
{
  int ret;
  struct io_uring_cqe *cqe;
  struct io_uring_sqe *sqe;

get:
  if ((sqe = io_uring_get_sqe (&ring)))
    return sqe;

  submit ();

  cqe = wait_cqe ();
  hnd_cqe (cqe);

  for (; !io_uring_peek_cqe (&ring, &cqe);)
    hnd_cqe (cqe);

  goto get;
}

void
prep_accept (void)
{
  union io_data ev;
  struct context *ctx;
  struct io_uring_sqe *sqe;

  ctx = memalloc (sizeof (struct context));

  ctx->fd = -1;
  ctx->in = ctx->out = NULL;
  ctx->inlen = ctx->outlen = 0;

  sqe = get_sqe ();
  io_uring_prep_accept (sqe, serv, &ctx->addr, &ctx->addrlen, 0);

  ev.type = EV_ACCEPT;
  ev.ctx = ptr2ctx (ctx);
  sqe->user_data = ev.u64;

  submit ();
}

void
prep_recv (int fd, struct context *ctx)
{
  if (fd < 0)
    return;

  if (ctx->fd < 0)
    ctx->fd = fd;

  if (ctx->in == NULL)
    ctx->in = memalloc (BLK_SIZE);

  union io_data ev;
  struct io_uring_sqe *sqe;

  sqe = get_sqe ();
  io_uring_prep_recv (sqe, ctx->fd, ctx->in, BLK_SIZE, 0);
  sqe->flags |= IOSQE_IO_LINK;

  ev.type = EV_RECV;
  ev.ctx = ptr2ctx (ctx);
  sqe->user_data = ev.u64;

  sqe = get_sqe ();
  io_uring_prep_link_timeout (sqe, &ts, 0);

  ev.type = EV_TIMEOUT;
  ev.ctx = ptr2ctx (ctx);
  sqe->user_data = ev.u64;

  submit ();
}

void
prep_send (int inlen, struct context *ctx)
{
  if (inlen <= 0)
    return drop_conn (ctx);

  ctx->inlen = inlen;
  if (ctx->out == NULL)
    ctx->out = memalloc (BLK_SIZE);

  ctx->outlen = ctx->inlen;
  memcpy (ctx->out, ctx->in, ctx->inlen);

  union io_data ev;
  struct io_uring_sqe *sqe;

  sqe = get_sqe ();
  io_uring_prep_send (sqe, ctx->fd, ctx->out, ctx->outlen, 0);

  ev.type = EV_SEND;
  ev.ctx = ptr2ctx (ctx);
  sqe->user_data = ev.u64;

  submit ();
}

void
check_send (int outlen, struct context *ctx)
{
  if (outlen != ctx->outlen)
    return drop_conn (ctx);
  prep_recv (0, ctx);
}

void
drop_conn (struct context *ctx)
{
  struct io_uring_sqe *sqe;

  sqe = get_sqe ();
  io_uring_prep_cancel_fd (sqe, ctx->fd, 0);
  submit ();

  close (ctx->fd);
  free (ctx->out);
  free (ctx->in);
  free (ctx);
}

void
hnd_cqe (struct io_uring_cqe *cqe)
{
  union io_data ev = { cqe->user_data };
  struct context *ctx = ctx2ptr (ev.ctx);

  switch (ev.type)
    {
    case EV_ACCEPT:
      prep_recv (cqe->res, ctx);
      prep_accept ();
      break;

    case EV_RECV:
      prep_send (cqe->res, ctx);
      break;

    case EV_SEND:
      check_send (cqe->res, ctx);
      break;
    }

  io_uring_cqe_seen (&ring, cqe);
}

int
main (void)
{
  int ret;

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons (3354),
    .sin_addr.s_addr = htonl (INADDR_ANY),
  };

  if (-1 == (serv = socket (AF_INET, SOCK_STREAM, 0)))
    error (1, "%s", strerror (errno));

  int opt = true;
  socklen_t len = sizeof (int);
  if (setsockopt (serv, SOL_SOCKET, SO_REUSEADDR, &opt, len) != 0)
    error (1, "%s", strerror (errno));

  if (0 != bind (serv, (void *)&addr, sizeof (addr)))
    error (1, "%s", strerror (errno));

  if (0 != listen (serv, 32))
    error (1, "%s", strerror (errno));

  if ((ret = io_uring_queue_init (QUEUE_DEPTH, &ring, 0)))
    error (1, "%s", strerror (-ret));

  prep_accept ();

  for (struct io_uring_cqe *cqe; (cqe = wait_cqe ());)
    hnd_cqe (cqe);
}
