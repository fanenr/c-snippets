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

  bool end;
  int num = 0;

  while (tok_next ())
    {
      num++;

      switch (tok.kind)
        {
        case TK_ID:
          printf ("%d 标识符 %s\n", num, tok.sval);
          break;

        case TK_NUM:
          printf ("%d 常数 %d\n", num, tok.ival);
          break;

        case TK_UNKNOW:
          printf ("%d 非法 %s\n", num, tok.sval);
          break;

        case TK_OP_ST + 1 ... TK_OP_ED - 1:
          printf ("%d 运算符 %s\n", num, tok.sval);
          break;

        case TK_KEY_ST + 1 ... TK_KEY_ED - 1:
          printf ("%d 保留字 %s\n", num, tok.sval);
          break;

        case TK_SEP_ST + 1 ... TK_SEP_ED - 1:
          printf ("%d 界符 %s\n", num, tok.sval);
          if (end && tok.kind == TK_SEP_DOT)
            goto end;
          break;

        default:
          error ("unkow type token");
        }

      end = (tok.kind == TK_KEY_END);
    }

end:
  fclose (ctx.file);
}

bool
tok_next (void)
{
  char ch;
  int len = 0;

  for (;;)
    { /* skip white space */
      ch = fgetc (ctx.file);
      if (isspace (ch))
        continue;
      if (ch != EOF)
        break;
      return false;
    }

  if (isalpha (ch))
    { /* match identifier */
      tok.kind = TK_ID;
      tok.sval[len++] = ch;

      for (;;)
        {
          ch = fgetc (ctx.file);
          if (isalnum (ch))
            tok.sval[len++] = ch;
          else
            {
              ungetc (ch, ctx.file);
              break;
            }
        }

      tok.sval[len] = 0;

      for (int i = TK_KEY_ST + 1; i < TK_KEY_ED; i++)
        {
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

      for (;;)
        {
          ch = fgetc (ctx.file);
          if (isdigit (ch))
            tok.ival = tok.ival * 10 + ch - '0';
          else
            {
              ungetc (ch, ctx.file);
              break;
            }
        }

      goto end;
    }

  switch (ch)
    {
    case ',':
      tok.kind = TK_SEP_COMMA;
      strcpy (tok.sval, ",");
      goto end;

    case ';':
      tok.kind = TK_SEP_SEMI;
      strcpy (tok.sval, ";");
      break;

    case '.':
      tok.kind = TK_SEP_DOT;
      strcpy (tok.sval, ".");
      break;

    case '+':
      tok.kind = TK_OP_PLUS;
      strcpy (tok.sval, "+");
      break;

    case '-':
      tok.kind = TK_OP_MINUS;
      strcpy (tok.sval, "-");
      break;

    case '*':
      tok.kind = TK_OP_TIMES;
      strcpy (tok.sval, "*");
      break;

    case '/':
      tok.kind = TK_OP_DIV;
      strcpy (tok.sval, "/");
      break;

    case '=':
      tok.kind = TK_OP_ASSIGN;
      strcpy (tok.sval, "=");
      break;

    case '#':
      tok.kind = TK_OP_SHARP;
      strcpy (tok.sval, "#");
      break;

    case '<':
      ch = fgetc (ctx.file);
      if (ch != '=')
        {
          ungetc (ch, ctx.file);
          tok.kind = TK_OP_LT;
          strcpy (tok.sval, "<");
        }
      else
        {
          tok.kind = TK_OP_LTEQ;
          strcpy (tok.sval, "<=");
        }
      break;

    case '>':
      ch = fgetc (ctx.file);
      if (ch != '=')
        {
          ungetc (ch, ctx.file);
          tok.kind = TK_OP_GT;
          strcpy (tok.sval, ">");
        }
      else
        {
          tok.kind = TK_OP_GTEQ;
          strcpy (tok.sval, ">=");
        }
      break;

    case ':':
      ch = fgetc (ctx.file);
      if (ch != '=')
        {
          ungetc (ch, ctx.file);
          tok.kind = TK_UNKNOW;
          strcpy (tok.sval, ":");
        }
      else
        {
          tok.kind = TK_OP_COLEQ;
          strcpy (tok.sval, ":=");
        }
      break;

    default:
      tok.kind = TK_UNKNOW;
      tok.sval[0] = ch;
      tok.sval[1] = 0;
      break;
    }

end:
  return true;
}
