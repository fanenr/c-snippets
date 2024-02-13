#define is_constexpr(EXP) (__builtin_constant_p (EXP))

#define is_same_type(TYPE1, TYPE2)                                            \
  (__builtin_types_compatible_p (TYPE1, TYPE2))

#define enable_if(CONST_EXP, THEN_EXP, ELSE_EXP)                              \
  (__builtin_choose_expr (CONST_EXP, THEN_EXP, ELSE_EXP))

#include <stdio.h>

int
main (void)
{
  int i = 1;
  const char *type
      = enable_if (is_same_type (typeof (i), int), "int", "not int");

  printf ("i is %s\n", type);
}
