#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void rand_init (void) __attribute__ ((constructor));
static long rand_long (long from, long to);
static char *rand_string (size_t len);

static inline void
rand_init (void)
{
  srand ((unsigned)time (NULL));
}

static inline long
rand_long (long from, long to)
{
  return rand () % (to - from) + from;
}

static inline char *
rand_string (size_t len)
{
  char *ret = malloc (sizeof (char) * (len + 1));
  if (!ret)
    return NULL;
  for (size_t i = 0; i < len; i++)
    ret[i] = (char)rand_long (32, 127);
  ret[len] = '\0';
  return ret;
}

int
main (void)
{
  for (unsigned i = 0; i < 100; i++)
    printf ("%s\n", rand_string (rand_long (8, 17)));
}
