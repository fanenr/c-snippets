#include <curl/curl.h>
#include <jansson.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum
{
  TYPE_MODELS,
  TYPE_INFO,
};

typedef struct result_t result_t;
typedef struct pack_t pack_t;

struct result_t
{
  bool ok;
  char key[55];
  char name[12];
  char balance[16];
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
}

static void
cleanup (void)
{
  curl_multi_cleanup (&multi);
  curl_global_cleanup ();
}

static void
get_models_hnd (pack_t *pack)
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

static size_t
get_models_cb (char *data, size_t size, size_t nmemb, void *ptr)
{
  pack_t *pack = ptr;
  size_t total = size * nmemb;

  if (got_models)
    return total;

  size_t oldsize = pack->size;
  size_t newsize = oldsize + total;
  char *newdata = realloc (pack->data, newsize);

  memcpy (newdata + oldsize, data, total);
  pack->size = newsize;
  pack->data = newdata;

  return total;
}

static void
get_models (result_t *res)
{
  CURL *hnd = curl_easy_init ();
  pack_t *pack = malloc (sizeof (pack_t));

#define URL_MODELS "https://api.siliconflow.cn/v1/models"

  curl_easy_setopt (hnd, CURLOPT_URL, URL_MODELS);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");

  static char hdr_key[80];
  sprintf (hdr_key, "authorization: Bearer %s", res->key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, hdr_key);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  curl_easy_setopt (hnd, CURLOPT_PRIVATE, pack);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, pack);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_models_cb);

  curl_multi_add_handle (multi, hnd);

  pack->res = res;

  pack->size = 0;
  pack->data = NULL;

  pack->hdrs = hdrs;
  pack->type = TYPE_MODELS;
}

static void
get_info_hnd (pack_t *pack)
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

static size_t
get_info_cb (char *data, size_t size, size_t nmemb, void *ptr)
{
  pack_t *pack = ptr;
  size_t total = size * nmemb;

  size_t oldsize = pack->size;
  size_t newsize = oldsize + total;
  char *newdata = realloc (pack->data, newsize);

  memcpy (newdata + oldsize, data, total);
  pack->size = newsize;
  pack->data = newdata;

  return total;
}

static void
get_info (result_t *res)
{
  CURL *hnd = curl_easy_init ();
  pack_t *pack = malloc (sizeof (pack_t));

#define URL_INFO "https://api.siliconflow.cn/v1/user/info"

  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt (hnd, CURLOPT_URL, URL_INFO);

  static char hdr_key[80];
  sprintf (hdr_key, "authorization: Bearer %s", res->key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, hdr_key);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  curl_easy_setopt (hnd, CURLOPT_PRIVATE, pack);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, pack);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_info_cb);

  curl_multi_add_handle (multi, hnd);

  pack->res = res;
  pack->type = TYPE_INFO;

  pack->size = 0;
  pack->data = NULL;

  pack->hdrs = hdrs;
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
      res->ok = false;

      results[num++] = res;
      get_models (res);
      get_info (res);
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
              get_models_hnd (pack);
            else if (pack->type == TYPE_INFO)
              get_info_hnd (pack);

          clean:
            curl_slist_free_all (pack->hdrs);
            free (pack->data);
            free (pack);

            curl_multi_remove_handle (multi, hnd);
            curl_easy_cleanup (hnd);
          }
    }

  FILE *result = fopen ("./result", "w");
  fputs ("key, name, balance\n", result);

  for (int i = 0; i < num; i++)
    {
      result_t *res = results[i];
      bool ok = res->ok;

      char *key = res->key;
      char *name = ok ? res->name : "";
      char *balance = ok ? res->balance : "";

      fprintf (result, "%s, %s, %s\n", key, name, balance);

      free (res);
    }
}
