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

typedef struct gen_t gen_t;
typedef struct token_t token_t;

struct token_t
{
  int kind;
  const char *sval;
};

#define TOKEN_IS_NT(TOK)                                                      \
  (TOKEN_NT_ST < (TOK)->kind && (TOK)->kind < TOKEN_NT_ED)

#define TOKEN_IS_T(TOK) (TOKEN_T_ST < (TOK)->kind && (TOK)->kind < TOKEN_T_ED)

struct gen_t
{
  int rsize;
  token_t *left;
  token_t **right;
  const char *sval;
};

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

token_t *ts[] = { &t_p, &t_a, &t_d, &t_e, &t_b };
int ts_size = sizeof (ts) / sizeof (ts[0]);

gen_t g_s = { .rsize = 3,
              .left = &nt_S,
              .sval = "S->aH#",
              .right = (token_t *[]){ &t_a, &nt_H, &t_p } };

gen_t g_h1 = { .rsize = 3,
               .left = &nt_H,
               .sval = "H->aMd",
               .right = (token_t *[]){ &t_a, &nt_M, &t_d } };

gen_t g_h2 = {
  .rsize = 1, .left = &nt_H, .sval = "H->d", .right = (token_t *[]){ &t_d }
};

gen_t g_m1 = { .rsize = 2,
               .left = &nt_M,
               .sval = "M->Ab",
               .right = (token_t *[]){ &nt_A, &t_b } };

gen_t g_m2
    = { .rsize = 0, .left = &nt_M, .sval = "M->", .right = (token_t *[]){} };

gen_t g_a1 = { .rsize = 2,
               .left = &nt_A,
               .sval = "A->aM",
               .right = (token_t *[]){ &t_a, &nt_M } };

gen_t g_a2 = {
  .rsize = 1, .left = &nt_A, .sval = "A->e", .right = (token_t *[]){ &t_e }
};

gen_t *gens[] = { &g_s, &g_h1, &g_h2, &g_m1, &g_m2, &g_a1, &g_a2 };
int gens_size = sizeof (gens) / sizeof (gens[0]);

bool nullable (token_t *nt);
bool nullable2 (gen_t *gen);

bool
nullable (token_t *nt)
{
  if (TOKEN_IS_T (nt))
    return false;

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = gens[i];
      if (nt != gen->left)
        continue;

      if (nullable2 (gen))
        return true;
    }

  return false;
}

bool
nullable2 (gen_t *gen)
{
  for (int i = 0; i < gen->rsize; i++)
    if (!nullable (gen->right[i]))
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

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = gens[i];

      if (tok != gen->left)
        continue;

      for (int i = 0; i < gen->rsize; i++)
        {
          token_t *r = gen->right[i];
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
first_of2 (gen_t *gen, token_t **result)
{
  int num = 0;
  token_t *temp[8];

  for (int i = 0; i < gen->rsize; i++)
    {
      token_t *r = gen->right[i];
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

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = gens[i];

      for (int i = 0; i < gen->rsize; i++)
        {
          token_t *r = gen->right[i];
          if (nt != r)
            continue;

          for (int j = i + 1; j < gen->rsize + 1; j++)
            {
              if (j == gen->rsize)
                {
                  follow_of (gen->left, temp);
                  for (token_t **ptr = temp; *ptr; ptr++)
                    result[num++] = *ptr;
                  break;
                }

              token_t *next = gen->right[j];
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

/* get next prediction production */
gen_t *
gen_of (token_t *nt, token_t *tok)
{
  token_t *first[16];
  token_t *follow[8];

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = gens[i];
      if (nt != gen->left)
        continue;

      first_of2 (gen, first);
      for (token_t **ptr = first; *ptr; ptr++)
        if (tok == *ptr)
          return gen;

      if (!nullable2 (gen))
        continue;

      follow_of (nt, follow);
      for (token_t **ptr = follow; *ptr; ptr++)
        if (tok == *ptr)
          return gen;
    }

  return NULL;
}

/* get next terminal token: NULL is returned if get '\n' */
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

struct
{
  int size;
  token_t *toks[24];
} stack;

void
stack_push (token_t *tok)
{
  stack.toks[stack.size++] = tok;
}

token_t *
stack_top (void)
{
  return stack.size ? stack.toks[stack.size - 1] : NULL;
}

void
stack_pop (void)
{
  if (stack.size > 0)
    stack.toks[--stack.size] = NULL;
}

void
print_info (void)
{
  token_t *first[16];
  token_t *follow[8];

  token_t *nts[] = { &nt_S, &nt_H, &nt_M, &nt_A };
  int nts_size = sizeof (nts) / sizeof (nts[0]);

  token_t *ts[] = { &t_p, &t_a, &t_d, &t_e, &t_b };
  int ts_size = sizeof (ts) / sizeof (ts[0]);

  printf ("token\t\tfirst\t\tfollow\n");
  for (int i = 0; i < nts_size; i++)
    {
      token_t *nt = nts[i];
      printf ("%s", nt->sval);

      printf ("\t\t");
      first_of (nt, first);
      for (token_t **ptr = first, *curr; (curr = *ptr); ptr++)
        printf ("%s ", curr->sval);

      printf ("\t\t");
      follow_of (nt, follow);
      for (token_t **ptr = follow, *curr; (curr = *ptr); ptr++)
        printf ("%s ", curr->sval);

      printf ("\n");
    }

  printf ("\n%-16sfirst\n", "gen");
  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = gens[i];
      token_t **toks = gen->right;
      printf ("%-16s", gen->sval);

      first_of2 (gen, first);
      for (token_t **ptr = first, *curr; (curr = *ptr); ptr++)
        printf ("%s ", curr->sval);

      printf ("\n");
    }

  printf ("\n%4s", "");
  for (int i = 0; i < ts_size; i++)
    printf ("%-10s", ts[i]->sval);

  printf ("\n");
  for (int i = 0; i < nts_size; i++)
    {
      token_t *nt = nts[i];
      printf ("%-4s", nt->sval);
      for (int j = 0; j < ts_size; j++)
        {
          token_t *t = ts[j];
          gen_t *gen = gen_of (nt, t);
          printf ("%-10s", gen ? gen->sval : "");
        }
      printf ("\n");
    }
  printf ("\n");
}

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
      if (!(top = stack_top ()))
        {
          tok = token_next ();
          stack_push (&nt_S);
          top = &nt_S;
        }

      if (TOKEN_IS_T (top))
        {
          if (tok != top)
            {
              printf ("syntax error\n");
              RESET ();
              continue;
            }

          stack_pop ();

          if (!stack.size)
            {
              printf ("parse ok\n");
              RESET ();
              continue;
            }

          tok = token_next ();
          continue;
        }

      stack_pop ();
      gen_t *gen;

      if (!(gen = gen_of (top, tok)))
        {
          printf ("syntax error\n");
          RESET ();
          continue;
        }

      for (int i = gen->rsize - 1; i >= 0; i--)
        stack_push (gen->right[i]);
    }
}
