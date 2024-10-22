#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <liburing.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLK_SIZE 4096
#define QUEUE_DEPTH 64

union io_data
{
  __u64 u64;
  struct
  {
    int op;
    unsigned len;
  };
};

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

#define submit(ring)                                                          \
  do                                                                          \
    {                                                                         \
      int ret;                                                                \
      if ((ret = io_uring_submit (ring)) < 0)                                 \
        error (1, "%s", strerror (-ret));                                     \
    }                                                                         \
  while (0)

#define wait_cqe(ring)                                                        \
  ({                                                                          \
    int ret;                                                                  \
    struct io_uring_cqe *cqe;                                                 \
    if ((ret = io_uring_wait_cqe (ring, &cqe)))                               \
      error (1, "%s", strerror (-ret));                                       \
    cqe;                                                                      \
  })

#define check_cqe(cqe)                                                        \
  do                                                                          \
    {                                                                         \
      union io_data data = { cqe->user_data };                                \
      if ((size_t)cqe->res != data.len)                                       \
        error (1, "failed to %s",                                             \
               (data.op == IORING_OP_READ) ? "read" : "write");               \
    }                                                                         \
  while (0)

size_t
sizeof_fd (int fd)
{
  struct stat stat;
  if (fstat (fd, &stat) != 0)
    error (1, "%s", strerror (errno));
  return stat.st_size;
}

struct io_uring_sqe *
get_sqe (struct io_uring *ring)
{
  int ret;
  struct io_uring_sqe *sqe;
  struct io_uring_cqe *cqe;

get:
  if ((sqe = io_uring_get_sqe (ring)))
    return sqe;

  submit (ring);
  cqe = wait_cqe (ring);
  check_cqe (cqe);
  io_uring_cqe_seen (ring, cqe);

  for (; !io_uring_peek_cqe (ring, &cqe); io_uring_cqe_seen (ring, cqe))
    check_cqe (cqe);

  goto get;
}

void
prep_copy (struct io_uring *ring, int rfd, int wfd, void *buff, unsigned len,
           off_t off)
{
  union io_data data;
  struct io_uring_sqe *sqe;

  sqe = get_sqe (ring);
  io_uring_prep_read (sqe, rfd, buff, len, off);
  sqe->flags |= IOSQE_IO_LINK;

  data.len = len;
  data.op = IORING_OP_READ;
  sqe->user_data = data.u64;

  sqe = get_sqe (ring);
  io_uring_prep_write (sqe, wfd, buff, len, off);

  data.len = len;
  data.op = IORING_OP_WRITE;
  sqe->user_data = data.u64;
}

int
main (int argc, char **argv)
{
  if (argc < 3)
    error (1, "Usage: %s <src> <dst>", argv[0]);

  int ret, rfd, wfd;
  struct io_uring ring;

  if ((rfd = open (argv[1], O_RDONLY)) < 0)
    error (1, "%s", strerror (errno));

  if ((wfd = open (argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0)
    error (1, "%s", strerror (errno));

  if ((ret = io_uring_queue_init (QUEUE_DEPTH, &ring, 0)))
    error (1, "%s", strerror (-ret));

  size_t size = sizeof_fd (rfd);
  size_t nblks = size / BLK_SIZE;

  if (size % BLK_SIZE)
    nblks++;

  void **blks;

  if (!(blks = malloc (nblks * sizeof (void *))))
    error (1, "%s", strerror (errno));

  for (size_t i = 0; i < nblks; i++)
    if (!(blks[i] = aligned_alloc (BLK_SIZE, BLK_SIZE)))
      error (1, "%s", strerror (errno));

  for (size_t i = 0; i < nblks; i++)
    {
      size_t len = size > BLK_SIZE ? BLK_SIZE : size;
      prep_copy (&ring, rfd, wfd, blks[i], len, i * BLK_SIZE);
      size -= len;
    }

  submit (&ring);

  for (struct io_uring_cqe *cqe;
       io_uring_sq_ready (&ring) || io_uring_cq_ready (&ring);
       io_uring_cqe_seen (&ring, cqe))
    {
      cqe = wait_cqe (&ring);
      check_cqe (cqe);
    }
}
