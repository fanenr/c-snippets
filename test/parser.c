/* README
 *
 * Although I have removed all language extensions for compatibility and only
 * use standard C, by all means compile with GCC if possible. Furthermore, the
 * files are encoded in UTF-8, so files garble is not my problem.
 */

/* build and run: gcc parser.c && ./a.out */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
  TOKEN_NT_ST,
  TOKEN_NT_S,
  TOKEN_NT_H,
  TOKEN_NT_M,
  TOKEN_NT_A,
  TOKEN_NT_ED,

  TOKEN_T_ST,
  TOKEN_T_P,
  TOKEN_T_A,
  TOKEN_T_D,
  TOKEN_T_E,
  TOKEN_T_B,
  TOKEN_T_ED,
};

typedef struct prod_t prod_t;
typedef struct token_t token_t;

struct token_t
{
  int kind;
  const char *sval;
};

struct prod_t
{
  int rsize;
  token_t *left;
  token_t **right;
  const char *sval;
};

struct
{
  int size;
  token_t *toks[24];
} stack;

/* non-terminal tokens */
token_t nt_S = { .kind = TOKEN_NT_S, .sval = "S" }; /* S */
token_t nt_H = { .kind = TOKEN_NT_H, .sval = "H" }; /* H */
token_t nt_M = { .kind = TOKEN_NT_M, .sval = "M" }; /* M */
token_t nt_A = { .kind = TOKEN_NT_A, .sval = "A" }; /* A */

token_t *nts[] = { &nt_S, &nt_H, &nt_M, &nt_A };
int nts_size = sizeof (nts) / sizeof (nts[0]);

/* terminal tokens */
token_t t_p = { .kind = TOKEN_T_P, .sval = "#" }; /* # */
token_t t_a = { .kind = TOKEN_T_A, .sval = "a" }; /* a */
token_t t_d = { .kind = TOKEN_T_D, .sval = "d" }; /* d */
token_t t_e = { .kind = TOKEN_T_E, .sval = "e" }; /* e */
token_t t_b = { .kind = TOKEN_T_B, .sval = "b" }; /* b */

token_t *ts[] = { &t_a, &t_d, &t_e, &t_b, &t_p };
int ts_size = sizeof (ts) / sizeof (ts[0]);

/* productions */
token_t *rg_s[] = { &t_a, &nt_H, &t_p };
prod_t g_s = { .rsize = 3, .left = &nt_S, .sval = "S->aH#", .right = rg_s };

token_t *rg_h1[] = { &t_a, &nt_M, &t_d };
prod_t g_h1 = { .rsize = 3, .left = &nt_H, .sval = "H->aMd", .right = rg_h1 };

token_t *rg_h2[] = { &t_d };
prod_t g_h2 = { .rsize = 1, .left = &nt_H, .sval = "H->d", .right = rg_h2 };

token_t *rg_m1[] = { &nt_A, &t_b };
prod_t g_m1 = { .rsize = 2, .left = &nt_M, .sval = "M->Ab", .right = rg_m1 };

token_t *rg_m2[] = { NULL };
prod_t g_m2 = { .rsize = 0, .left = &nt_M, .sval = "M->", .right = rg_m2 };

token_t *rg_a1[] = { &t_a, &nt_M };
prod_t g_a1 = { .rsize = 2, .left = &nt_A, .sval = "A->aM", .right = rg_a1 };

token_t *rg_a2[] = { &t_e };
prod_t g_a2 = { .rsize = 1, .left = &nt_A, .sval = "A->e", .right = rg_a2 };

prod_t *prods[] = { &g_s, &g_h1, &g_h2, &g_m1, &g_m2, &g_a1, &g_a2 };
int prods_size = sizeof (prods) / sizeof (prods[0]);

/* print first, follow sets and LL(1) prediction table */
void print_info (void);

/* get next token: NULL is returned if reach '\n' */
token_t *token_next (void);

/* check whether a production is nullable */
bool nullable2 (prod_t *prod);

/* check whether a token is nullable */
bool nullable (token_t *nt);

/* get the first set of a token */
void first_of (token_t *tok, token_t **result);

/* get the first set of a production */
void first_of2 (prod_t *prod, token_t **result);

/* get the follow set of a non-terminal token */
void follow_of (token_t *nt, token_t **result);

/* get the next production according to nt and tok */
prod_t *prod_of (token_t *nt, token_t *tok);

#define TOKEN_IS_T(TOK) (TOKEN_T_ST < (TOK)->kind && (TOK)->kind < TOKEN_T_ED)

#define STACK_TOP() (stack.size ? stack.toks[stack.size - 1] : NULL)

#define STACK_PUSH(TOK)                                                       \
  do                                                                          \
    stack.toks[stack.size++] = (TOK);                                         \
  while (0)

#define STACK_POP()                                                           \
  do                                                                          \
    if (stack.size)                                                           \
      stack.toks[--stack.size] = NULL;                                        \
  while (0)

#define RESET()                                                               \
  do                                                                          \
    {                                                                         \
      stack.size = 0;                                                         \
      for (; tok; tok = token_next ())                                        \
	;                                                                     \
    }                                                                         \
  while (0)

int
main (void)
{
  print_info ();

  for (token_t *tok, *top;;)
    {
      if (!(top = STACK_TOP ()))
	{
	  tok = token_next ();
	  STACK_PUSH (&nt_S);
	  top = &nt_S;
	}

      if (TOKEN_IS_T (top))
	{
	  if (tok != top)
	    {
	      printf ("syntax error\n\n");
	      RESET ();
	      continue;
	    }

	  STACK_POP ();
	  if (!stack.size)
	    {
	      printf ("parse ok\n\n");
	      RESET ();
	      continue;
	    }

	  tok = token_next ();
	  continue;
	}

      prod_t *prod;
      STACK_POP ();
      if (!(prod = prod_of (top, tok)))
	{
	  printf ("syntax error\n\n");
	  RESET ();
	  continue;
	}

      for (int i = prod->rsize - 1; i >= 0; i--)
	STACK_PUSH (prod->right[i]);
    }
}

void
print_info (void)
{
  token_t *first[16];
  token_t *follow[8];

  puts ("token\t\tfirst\t\tfollow");
  puts ("--------------------------------------");
  for (int i = 0; i < nts_size; i++)
    {
      token_t *nt = nts[i];
      printf ("%s", nt->sval);

      printf ("\t\t");
      first_of (nt, first);
      for (token_t **ptr = first; *ptr; ptr++)
	printf ("%s ", (*ptr)->sval);

      printf ("\t\t");
      follow_of (nt, follow);
      for (token_t **ptr = follow; *ptr; ptr++)
	printf ("%s ", (*ptr)->sval);
      puts ("");
    }

  printf ("\n%-16sfirst\n", "production");
  puts ("---------------------");
  for (int i = 0; i < prods_size; i++)
    {
      prod_t *prod = prods[i];
      token_t **toks = prod->right;
      printf ("%-16s", prod->sval);

      first_of2 (prod, first);
      for (token_t **ptr = first; *ptr; ptr++)
	printf ("%s ", (*ptr)->sval);
      puts ("");
    }

  printf ("\n    ");
  for (int i = 0; i < ts_size; i++)
    printf ("%-10s", ts[i]->sval);

  puts ("\n---------------------------------------------");
  for (int i = 0; i < nts_size; i++)
    {
      token_t *nt = nts[i];
      printf ("%-4s", nt->sval);
      for (int j = 0; j < ts_size; j++)
	{
	  prod_t *prod = prod_of (nt, ts[j]);
	  printf ("%-10s", prod ? prod->sval : "");
	}
      puts ("");
    }
  puts ("");
}

bool
nullable (token_t *nt)
{
  if (TOKEN_IS_T (nt))
    return false;

  for (int i = 0; i < prods_size; i++)
    {
      prod_t *prod = prods[i];
      if (nt != prod->left)
	continue;

      if (nullable2 (prod))
	return true;
    }

  return false;
}

bool
nullable2 (prod_t *prod)
{
  for (int i = 0; i < prod->rsize; i++)
    if (!nullable (prod->right[i]))
      return false;
  return true;
}

void
first_of (token_t *tok, token_t **result)
{
  int num = 0;
  token_t *temp[8];
  if (TOKEN_IS_T (tok))
    {
      result[num++] = tok;
      goto ret;
    }

  for (int i = 0; i < prods_size; i++)
    {
      prod_t *prod = prods[i];
      if (tok != prod->left)
	continue;

      for (int i = 0; i < prod->rsize; i++)
	{
	  token_t *r = prod->right[i];
	  first_of (r, temp);

	  for (token_t **ptr = temp; *ptr; ptr++)
	    result[num++] = *ptr;
	  if (!nullable (r))
	    break;
	}
    }

ret:
  result[num] = NULL;
}

void
first_of2 (prod_t *prod, token_t **result)
{
  int num = 0;
  token_t *temp[8];

  for (int i = 0; i < prod->rsize; i++)
    {
      token_t *r = prod->right[i];
      first_of (r, temp);

      for (token_t **ptr = temp; *ptr; ptr++)
	result[num++] = *ptr;
      if (!nullable (r))
	break;
    }

ret:
  result[num] = NULL;
}

void
follow_of (token_t *nt, token_t **result)
{
  int num = 0;
  token_t *temp[8];
  if (TOKEN_IS_T (nt))
    goto ret;

  for (int i = 0; i < prods_size; i++)
    {
      prod_t *prod = prods[i];

      for (int i = 0; i < prod->rsize; i++)
	{
	  if (nt != prod->right[i])
	    continue;

	  for (int j = i + 1; j < prod->rsize + 1; j++)
	    {
	      if (j == prod->rsize)
		{
		  follow_of (prod->left, temp);
		  for (token_t **ptr = temp; *ptr; ptr++)
		    result[num++] = *ptr;
		  break;
		}

	      token_t *next = prod->right[j];
	      first_of (next, temp);

	      for (token_t **ptr = temp; *ptr; ptr++)
		result[num++] = *ptr;
	      if (!nullable (next))
		break;
	    }
	}
    }

ret:
  result[num] = NULL;
}

prod_t *
prod_of (token_t *nt, token_t *tok)
{
  token_t *first[16];
  token_t *follow[8];

  for (int i = 0; i < prods_size; i++)
    {
      prod_t *prod = prods[i];
      if (nt != prod->left)
	continue;

      first_of2 (prod, first);
      for (token_t **ptr = first; *ptr; ptr++)
	if (tok == *ptr)
	  return prod;

      if (!nullable2 (prod))
	continue;

      follow_of (nt, follow);
      for (token_t **ptr = follow; *ptr; ptr++)
	if (tok == *ptr)
	  return prod;
    }

  return NULL;
}

token_t *
token_next (void)
{
  char ch = getchar ();
  switch (ch)
    {
    case '\n':
      return NULL;
    case '#':
      return &t_p;
    case 'a':
      return &t_a;
    case 'd':
      return &t_d;
    case 'e':
      return &t_e;
    case 'b':
      return &t_b;
    }
  printf ("unknown token %c\n", ch);
  exit (1);
}
