#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define error(FMT, ...)                                                       \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, FMT, ##__VA_ARGS__);                                   \
      fprintf (stderr, "\n");                                                 \
      exit (1);                                                               \
    }                                                                         \
  while (0)

enum
{
  TK_ID,
  TK_NUM,
  TK_UNKNOW,

  TK_KEY_ST,
  TK_KEY_IF,
  TK_KEY_DO,
  TK_KEY_VAR,
  TK_KEY_ODD,
  TK_KEY_END,
  TK_KEY_THEN,
  TK_KEY_CALL,
  TK_KEY_READ,
  TK_KEY_CONST,
  TK_KEY_BEGIN,
  TK_KEY_WHILE,
  TK_KEY_WRITE,
  TK_KEY_WUFAN,
  TK_KEY_PROCEDURE,
  TK_KEY_ED,

  TK_OP_ST,
  TK_OP_LT,
  TK_OP_GT,
  TK_OP_DIV,
  TK_OP_LTEQ,
  TK_OP_GTEQ,
  TK_OP_PLUS,
  TK_OP_MINUS,
  TK_OP_TIMES,
  TK_OP_SHARP,
  TK_OP_COLEQ,
  TK_OP_ASSIGN,
  TK_OP_ED,

  TK_SEP_ST,
  TK_SEP_DOT,
  TK_SEP_SEMI,
  TK_SEP_COMMA,
  TK_SEP_ED,
};

#define TK_IS_OP(KIND) (TK_OP_ST < (KIND) && (KIND) < TK_OP_ED)
#define TK_IS_KEY(KIND) (TK_KEY_ST < (KIND) && (KIND) < TK_KEY_ED)
#define TK_IS_SEP(KIND) (TK_SEP_ST < (KIND) && (KIND) < TK_SEP_ED)

const char *tk_keys[] = {
  [TK_KEY_IF] = "if",       [TK_KEY_DO] = "do",
  [TK_KEY_VAR] = "var",     [TK_KEY_ODD] = "odd",
  [TK_KEY_END] = "end",     [TK_KEY_CALL] = "call",
  [TK_KEY_THEN] = "then",   [TK_KEY_READ] = "read",
  [TK_KEY_CONST] = "const", [TK_KEY_BEGIN] = "begin",
  [TK_KEY_WHILE] = "while", [TK_KEY_WRITE] = "write",
  [TK_KEY_WUFAN] = "wufan", [TK_KEY_PROCEDURE] = "procedure",
};

struct
{
  int kind; /* TK_XXX */
  union
  {
    int ival;      /* for TK_NUM */
    char sval[64]; /* for other  */
  };
} tok;

struct
{
  FILE *file;
} ctx;

/* scan next token: return false represents reach EOF */
bool tok_next (void);

int
main (int argc, char **args)
{
  if (argc != 2)
    error ("usage: %s input-file.txt", args[0]);

  if (!(ctx.file = fopen (args[1], "r")))
    error ("can not open file %s", args[1]);

  int kind;
  int num = 0;

  while (tok_next ())
    {
      num++;

      switch ((kind = tok.kind))
        {
        case TK_ID:
          printf ("%3d 标识符 %-24s", num, tok.sval);
          break;

        case TK_NUM:
          printf ("%3d 常数   %-24d", num, tok.ival);
          break;

        case TK_UNKNOW:
          printf ("%3d 非法   %-24s", num, tok.sval);
          break;

        default:
          if (TK_IS_OP (kind))
            printf ("%3d 运算符 %-24s", num, tok.sval);
          else if (TK_IS_KEY (kind))
            printf ("%3d 保留字 %-24s", num, tok.sval);
          else if (TK_IS_SEP (kind))
            printf ("%3d 界符   %-24s", num, tok.sval);
          else
            error ("unknow type token");
        }

      putchar (num % 2 ? '\t' : '\n');
    }

end:
  fclose (ctx.file);
}

bool
tok_next (void)
{
  char ch;
  int len = 0;
  FILE *file = ctx.file;

  for (; isspace ((ch = fgetc (file)));)
    ; /* skip white space */

  if (ch == EOF)
    return false;

  if (isalpha (ch))
    { /* match identifier */
      tok.kind = TK_ID;
      tok.sval[len++] = ch;

      for (; isalnum ((ch = fgetc (file)));)
        tok.sval[len++] = ch;

      ungetc (ch, file);
      tok.sval[len] = 0;

      for (int i = TK_KEY_ST + 1; i < TK_KEY_ED; i++)
        { /* determine whether it's a keyword */
          if (strcmp (tok.sval, tk_keys[i]) != 0)
            continue;
          tok.kind = i;
          break;
        }

      goto end;
    }

  if (isdigit (ch))
    { /* match num */
      tok.kind = TK_NUM;
      tok.ival = ch - '0';

      for (; isdigit ((ch = fgetc (file)));)
        tok.ival = tok.ival * 10 + ch - '0';

      ungetc (ch, file);
      goto end;
    }

  int kind;
  const char *sval;

  switch (ch)
    {
    case ',':
      kind = TK_SEP_COMMA;
      sval = ",";
      break;

    case ';':
      kind = TK_SEP_SEMI;
      sval = ";";
      break;

    case '.':
      kind = TK_SEP_DOT;
      sval = ".";
      break;

    case '+':
      kind = TK_OP_PLUS;
      sval = "+";
      break;

    case '-':
      kind = TK_OP_MINUS;
      sval = "-";
      break;

    case '*':
      kind = TK_OP_TIMES;
      sval = "*";
      break;

    case '/':
      kind = TK_OP_DIV;
      sval = "/";
      break;

    case '=':
      kind = TK_OP_ASSIGN;
      sval = "=";
      break;

    case '#':
      kind = TK_OP_SHARP;
      sval = "#";
      break;

    case '<':
      if ((ch = fgetc (file)) != '=')
        {
          ungetc (ch, file);
          kind = TK_OP_LT;
          sval = "<";
        }
      else
        {
          kind = TK_OP_LTEQ;
          sval = "<=";
        }
      break;

    case '>':
      if ((ch = fgetc (file)) != '=')
        {
          ungetc (ch, file);
          kind = TK_OP_GT;
          sval = ">";
        }
      else
        {
          kind = TK_OP_GTEQ;
          sval = ">=";
        }
      break;

    case ':':
      if ((ch = fgetc (file)) != '=')
        {
          ungetc (ch, file);
          kind = TK_UNKNOW;
          sval = ":";
        }
      else
        {
          kind = TK_OP_COLEQ;
          sval = ":=";
        }
      break;

    default:
      tok.kind = TK_UNKNOW;
      tok.sval[0] = ch;
      tok.sval[1] = 0;
      goto end;
    }

  strcpy (tok.sval, sval);
  tok.kind = kind;

end:
  return true;
}
