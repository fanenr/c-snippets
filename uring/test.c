#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <liburing.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLK_SIZE 4096
#define QUEUE_DEPTH 64

int
main (void)
{
  struct io_uring ring;
  io_uring_queue_init (QUEUE_DEPTH, &ring, 0);

  char buff[BLK_SIZE];
  struct io_uring_sqe *sqe;

  sqe = io_uring_get_sqe (&ring);
  io_uring_prep_read (sqe, STDIN_FILENO, buff, 8, 0);
  sqe->flags |= IOSQE_IO_LINK;
  sqe->user_data = 1;

  struct __kernel_timespec ts;
  ts.tv_nsec = 0;
  ts.tv_sec = 2;

  sqe = io_uring_get_sqe (&ring);
  io_uring_prep_link_timeout (sqe, &ts, 0);
  sqe->user_data = 2;

  io_uring_submit (&ring);

  struct io_uring_cqe *cqe;
  io_uring_wait_cqe (&ring, &cqe);
  printf ("%llu\n", cqe->user_data);
}
