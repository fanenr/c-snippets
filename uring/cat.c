#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <liburing.h>
#include <sys/stat.h>
#include <unistd.h>

#define QUEUE_SIZE 64
#define BLK_SIZE 4096

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

#define check_cqe(cqe)                                                        \
  do                                                                          \
    {                                                                         \
      union io_data data = { cqe->user_data };                                \
      if (cqe->res != data.data.len)                                          \
        error (1, "failed to read");                                          \
    }                                                                         \
  while (0)

union io_data
{
  __u64 u64;
  struct
  {
    int op;
    unsigned len;
  } data;
};

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
  struct io_uring_cqe *cqe;
  struct io_uring_sqe *sqe;

get:
  if ((sqe = io_uring_get_sqe (ring)))
    return sqe;

  submit (ring);

  if ((ret = io_uring_wait_cqe (ring, &cqe)))
    error (1, "%s", strerror (-ret));

  check_cqe (cqe);
  io_uring_cqe_seen (ring, cqe);

  for (; !io_uring_peek_cqe (ring, &cqe); io_uring_cqe_seen (ring, cqe))
    check_cqe (cqe);

  goto get;
}

void
prep_read (struct io_uring *ring, int fd, void *buff, unsigned len, off_t off)
{
  union io_data data;
  struct io_uring_sqe *sqe;

  sqe = get_sqe (ring);
  io_uring_prep_read (sqe, fd, buff, len, off);

  data.data.len = len;
  sqe->user_data = data.u64;
}

int
main (int argc, char **argv)
{
  if (argc < 2)
    error (1, "Usage: %s <file>", argv[0]);

  int ret, fd;
  struct io_uring ring;

  if ((fd = open (argv[1], O_RDONLY)) < 0)
    error (1, "%s", strerror (errno));

  if ((ret = io_uring_queue_init (QUEUE_SIZE, &ring, 0)))
    error (1, "%s", strerror (-ret));

  size_t size = sizeof_fd (fd);
  size_t nblks = size / BLK_SIZE;

  if (size % BLK_SIZE)
    nblks++;

  void **blks;

  if (!(blks = malloc (nblks * sizeof (void *))))
    error (1, "%s", strerror (errno));

  for (size_t i = 0; i < nblks; i++)
    if (!(blks[i] = aligned_alloc (BLK_SIZE, BLK_SIZE)))
      error (1, "%s", strerror (errno));

  for (size_t i = 0, copy = size; i < nblks; i++)
    {
      size_t len = copy > BLK_SIZE ? BLK_SIZE : copy;
      prep_read (&ring, fd, blks[i], len, i * BLK_SIZE);
      copy -= len;
    }

  submit (&ring);

  for (struct io_uring_cqe *cqe;
       io_uring_sq_ready (&ring) || io_uring_cq_ready (&ring);
       io_uring_cqe_seen (&ring, cqe))
    {
      if ((ret = io_uring_wait_cqe (&ring, &cqe)))
        error (1, "%s", strerror (-ret));
      check_cqe (cqe);
    }

  for (size_t i = 0; i < nblks; i++)
    {
      size_t len = size > BLK_SIZE ? BLK_SIZE : size;
      if (fwrite (blks[i], 1, len, stdout) != len)
        error (1, "%s", strerror (errno));
      size -= len;
    }
}
