#include <curl/curl.h>
#include <jansson.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum
{
  TYPE_INFO,
  TYPE_MODELS,
  TYPE_CHAT_BASE,
  TYPE_CHAT_ADVANCED,
};

typedef struct result_t result_t;
typedef struct pack_t pack_t;

struct result_t
{
  bool ok;
  char key[55];
  char name[12];
  char balance[16];

  bool base;
  bool advanced;
};

struct pack_t
{
  int type;
  result_t *res;

  char *data;
  size_t size;

  struct curl_slist *hdrs;
};

static void init (void);
static void cleanup (void);
static void work (const char *path);

static CURLM *multi;
static bool got_models;
static char *chat_base_data;
static char *chat_advanced_data;

int
main (int argc, char **argv)
{
  init ();
  work (argc > 1 ? argv[1] : "./keys");
  cleanup ();
}

static void
init (void)
{
  curl_global_init (CURL_GLOBAL_ALL);
  multi = curl_multi_init ();

  json_t *base
      = json_pack ("{s:s,s:[{s:s,s:s}]}", "model", "Qwen/Qwen2-72B-Instruct",
                   "messages", "role", "user", "content", "AI 是什么？");
  json_t *advanced
      = json_pack ("{s:s,s:[{s:s,s:s}]}", "model",
                   "meta-llama/Meta-Llama-3.1-405B-Instruct", "messages",
                   "role", "user", "content", "AI 是什么？");

  chat_base_data = json_dumps (base, 0);
  chat_advanced_data = json_dumps (advanced, 0);

  json_decref (base);
  json_decref (advanced);
}

static void
cleanup (void)
{
  free (chat_advanced_data);
  free (chat_base_data);
  curl_multi_cleanup (&multi);
  curl_global_cleanup ();
}

static size_t
callback (char *data, size_t size, size_t nmemb, void *ptr)
{
  pack_t *pack = ptr;
  size_t total = size * nmemb;

  if (pack->type == TYPE_MODELS && got_models)
    return total;

  size_t oldsize = pack->size;
  size_t newsize = oldsize + total;
  char *newdata = realloc (pack->data, newsize);

  memcpy (newdata + oldsize, data, total);
  pack->size = newsize;
  pack->data = newdata;

  return total;
}

static inline void
test (result_t *res, int type)
{
  static const char *urls[] = {
    [TYPE_INFO] = "https://api.siliconflow.cn/v1/user/info",
    [TYPE_MODELS] = "https://api.siliconflow.cn/v1/models",
    [TYPE_CHAT_BASE... TYPE_CHAT_ADVANCED]
    = "https://api.siliconflow.cn/v1/chat/completions",
  };

  CURL *hnd = curl_easy_init ();
  pack_t *pack = malloc (sizeof (pack_t));

  static char hdr_key[80];
  sprintf (hdr_key, "authorization: Bearer %s", res->key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, hdr_key);
  hdrs = curl_slist_append (hdrs, "accept: application/json");
  hdrs = curl_slist_append (hdrs, "content-type: application/json");

  curl_easy_setopt (hnd, CURLOPT_PRIVATE, pack);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, pack);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, callback);

  const char *method = "GET";
  const char *url = urls[type];

  switch (pack->type = type)
    {
    case TYPE_CHAT_BASE:
      curl_easy_setopt (hnd, CURLOPT_POSTFIELDS, chat_base_data);
      method = "POST";
      break;
    case TYPE_CHAT_ADVANCED:
      curl_easy_setopt (hnd, CURLOPT_POSTFIELDS, chat_advanced_data);
      method = "POST";
      break;
    }

  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, method);

  curl_multi_add_handle (multi, hnd);
  pack->data = NULL;
  pack->hdrs = hdrs;
  pack->res = res;
  pack->size = 0;
}

static void
hnd_models (pack_t *pack)
{
  json_t *json = json_loadb (pack->data, pack->size, 0, NULL);

  if (json)
    {
      FILE *models = fopen ("./models", "w");
      json_t *arr = json_object_get (json, "data");

      for (size_t n = json_array_size (arr), i = 0; i < n; i++)
        {
          json_t *id = json_object_get (json_array_get (arr, i), "id");
          fputs (json_string_value (id), models);
          fputc ('\n', models);
        }

      json_decref (json);
      got_models = true;
      fclose (models);
    }
}

static void
hnd_info (pack_t *pack)
{
  result_t *res = pack->res;
  json_t *json = json_loadb (pack->data, pack->size, 0, NULL);

  if (json)
    {
      json_t *obj = json_object_get (json, "data");
      json_t *name = json_object_get (obj, "name");
      json_t *balance = json_object_get (obj, "balance");

      strcpy (res->balance, json_string_value (balance));
      strcpy (res->name, json_string_value (name));
      json_decref (json);
      res->ok = true;
    }
}

static inline void
hnd_chat (pack_t *pack)
{
  switch (pack->type)
    {
    case TYPE_CHAT_BASE:
      pack->res->base = true;
      break;
    case TYPE_CHAT_ADVANCED:
      pack->res->advanced = true;
      break;
    }
}

static void
work (const char *path)
{
  int num = 0;
  result_t *results[1024];
  FILE *file = fopen (path, "r");

  for (char key[64]; fgets (key, sizeof (key), file);)
    {
      if (key[0] == '\n')
        continue;

      size_t len = strlen (key);
      key[len - 1] = '\0';

      result_t *res = malloc (sizeof (result_t));
      memcpy (res->key, key, len + 1);
      res->advanced = false;
      res->base = false;
      res->ok = false;

      results[num++] = res;
      test (res, TYPE_INFO);
      test (res, TYPE_MODELS);
      test (res, TYPE_CHAT_BASE);
      test (res, TYPE_CHAT_ADVANCED);
    }

  for (int n = 1, r; n;)
    {
      curl_multi_perform (multi, &n);
      curl_multi_poll (multi, NULL, 0, 1000, NULL);

      for (struct CURLMsg *m; (m = curl_multi_info_read (multi, &r));)
        if (m->msg == CURLMSG_DONE)
          {
            long code;
            pack_t *pack;
            CURL *hnd = m->easy_handle;

            curl_easy_getinfo (hnd, CURLINFO_PRIVATE, &pack);
            curl_easy_getinfo (hnd, CURLINFO_RESPONSE_CODE, &code);

            if (code != 200)
              goto clean;

            if (pack->type == TYPE_MODELS && !got_models)
              hnd_models (pack);
            else if (pack->type == TYPE_INFO)
              hnd_info (pack);
            else
              hnd_chat (pack);

          clean:
            curl_slist_free_all (pack->hdrs);
            free (pack->data);
            free (pack);

            curl_multi_remove_handle (multi, hnd);
            curl_easy_cleanup (hnd);
          }
    }

  FILE *result = fopen ("./result", "w");
  fputs ("key, name, balance, base, advanced\n", result);

  for (int i = 0; i < num; i++)
    {
      result_t *res = results[i];
      bool ok = res->ok;

      char *key = res->key;
      char *name = ok ? res->name : "";
      char *balance = ok ? res->balance : "";

      char base = res->base ? 'y' : 'n';
      char advanced = res->advanced ? 'y' : 'n';

      fprintf (result, "%s, %s, %s, %c, %c\n", key, name, balance, base,
               advanced);

      free (res);
    }
}
