#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define error(fmt, ...)                                                       \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "%s:%s:%d: error: ", __FILE__, __FUNCTION__,           \
	       __LINE__);                                                     \
      fprintf (stderr, (fmt), ##__VA_ARGS__);                                 \
      fprintf (stderr, "\n");                                                 \
      exit (1);                                                               \
    }                                                                         \
  while (0)

#define error_if(expr, fmt, ...)                                              \
  do                                                                          \
    if (expr)                                                                 \
      error (fmt, ##__VA_ARGS__);                                             \
  while (0);

#define error_ifn(expr, fmt, ...)                                             \
  do                                                                          \
    if (!(expr))                                                              \
      error (fmt, ##__VA_ARGS__);                                             \
  while (0);

#define error_eq(a, b, fmt, ...)                                              \
  do                                                                          \
    if ((a) == (b))                                                           \
      error (fmt, ##__VA_ARGS__);                                             \
  while (0)

#define error_neq(a, b, fmt, ...)                                             \
  do                                                                          \
    if ((a) != (b))                                                           \
      error (fmt, ##__VA_ARGS__);                                             \
  while (0)

int
main (void)
{
  FILE *file;

  /* 1 */
  if (!(file = fopen ("a.txt", "r")))
    error ("%s", strerror (errno));

  /* 2 */
  error_ifn (({ file = fopen ("a.txt", "r"); }), "%s", strerror (errno));

  /* 3 */
  error_eq (NULL, ({ file = fopen ("a.txt", "r"); }), "%s", strerror (errno));
}
