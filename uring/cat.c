#include <assert.h>
#include <liburing.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
main (int argc, char **argv)
{
  int fd;
  void *buff;
  const char *path;
  struct io_uring_sqe *sqe;
  struct io_uring_cqe *cqe;

  assert (argc >= 2);

  struct io_uring ring;
  assert (io_uring_queue_init (8, &ring, 0) == 0);

  path = argv[1];
  assert ((fd = open (path, O_RDONLY)) >= 0);

  struct stat stat;
  assert (fstat (fd, &stat) == 0);

  size_t size = stat.st_size;
  assert ((buff = malloc (size)));

  sqe = io_uring_get_sqe (&ring);
  io_uring_prep_read (sqe, fd, buff, size, 0);
  sqe->flags |= IOSQE_IO_LINK;

  sqe = io_uring_get_sqe (&ring);
  io_uring_prep_write (sqe, STDOUT_FILENO, buff, size, 0);

  assert (io_uring_submit (&ring) == 2);

  for (int i = 0; i < 2; i++)
    {
      assert (io_uring_wait_cqe (&ring, &cqe) == 0);
      assert (cqe->res >= 0);
      io_uring_cqe_seen (&ring, cqe);
    }

  io_uring_queue_exit (&ring);
  free (buff);
  close (fd);
}
