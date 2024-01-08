#include <stdio.h>

#define defer_cat_(s1, s2) s1##s2
#define defer_cat(s1, s2) defer_cat_ (s1, s2)
#define defer(code)                                                           \
  inline void defer_cat (defer_func_,                                         \
                         __LINE__) (char *ptr __attribute__ ((unused)))       \
  {                                                                           \
    code;                                                                     \
  };                                                                          \
  char defer_cat (defer_var_, __LINE__)                                       \
      __attribute__ ((cleanup (defer_cat (defer_func_, __LINE__)), unused));

#define dtor(func) __attribute__ ((cleanup (func)))

int
main ()
{
  /* nested function */
  inline void clean_ch (char *ch) { printf ("%c", *ch); }

  /* defer */
  defer (printf ("e"));

  /* destructor */
  char ch1 dtor (clean_ch) = 'c';
  char ch2 dtor (clean_ch) = 'b';
  char ch3 = ({
    char tmp;
    if (ch1 == 'c')
      tmp = ch1;
    else
      tmp = ch2;
    tmp;
  });
  printf ("%c", ch3);
}