#include <liburing.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int
main (void)
{
  struct io_uring ring;
  struct io_uring_sqe *sqe;
  struct io_uring_cqe *cqe;

  const char *file = "a.txt";
  const char *data = "Hello World";
  mode_t mode = O_RDWR | O_CREAT | O_TRUNC;

  int fd = open (file, mode, 0644);

  io_uring_queue_init (32, &ring, 0);

  sqe = io_uring_get_sqe (&ring);
  io_uring_prep_write (sqe, fd, data, 11, 0);
  sqe->flags |= IOSQE_IO_LINK;
  sqe->user_data = 1;

  sqe = io_uring_get_sqe (&ring);
  io_uring_prep_close (sqe, fd);
  sqe->user_data = 2;

  io_uring_submit (&ring);

  for (int i = 0; i < 2; i++)
    {
      io_uring_wait_cqe (&ring, &cqe);

      if (cqe->res < 0)
        printf ("%llu: %s\n", cqe->user_data, strerror (-cqe->res));

      io_uring_cqe_seen (&ring, cqe);
    }

  io_uring_queue_exit (&ring);
}
