#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <liburing.h>
#include <sys/stat.h>
#include <unistd.h>

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

#define QUEUE_SIZE 16
#define BLK_SIZE 4096

#define submit(ring)                                                          \
  do                                                                          \
    {                                                                         \
      int ret;                                                                \
      if ((ret = io_uring_submit (ring)) < 0)                                 \
        error (1, "%s", strerror (-ret));                                     \
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

void
prep_read (struct io_uring *ring, int fd, void *buff, unsigned len, off_t off)
{
  struct io_uring_sqe *sqe;
  if ((sqe = io_uring_get_sqe (ring)))
    return io_uring_prep_read (sqe, fd, buff, len, off);

  submit (ring);

  sqe = io_uring_get_sqe (ring);
  io_uring_prep_read (sqe, fd, buff, len, off);
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

  for (size_t i = 0; i < nblks; i++)
    prep_read (&ring, fd, blks[i], BLK_SIZE, i * BLK_SIZE);

  submit (&ring);

  for (size_t i = 0; i < nblks; i++)
    {
      struct io_uring_cqe *cqe;
      if ((ret = io_uring_wait_cqe (&ring, &cqe)))
        error (1, "%s", strerror (-ret));
      io_uring_cqe_seen (&ring, cqe);
    }

  for (size_t i = 0; i < nblks; i++)
    {
      size_t buff_size = size > BLK_SIZE ? BLK_SIZE : size;
      if (fwrite (blks[i], 1, buff_size, stdout) != buff_size)
        error (1, "%s", strerror (errno));
      size -= buff_size;
    }
}
