#include <curl/curl.h>
#include <jansson.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void init (void);
static void cleanup (void);
static void work (const char *path);

static FILE *result;
static FILE *models;
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
  assert (multi);

  remove ("./result");
  remove ("./models");

  result = fopen ("./result", "a");
  models = fopen ("./models", "w");
  assert (result && models);

  fputs ("key, name, balance\n", result);
}

static void
cleanup (void)
{
  fclose (models);
  fclose (result);
  curl_multi_cleanup (&multi);
  curl_global_cleanup ();
}

enum
{
  TYPE_MODELS,
  TYPE_INFO,
};

typedef struct pack_t pack_t;

struct pack_t
{
  int type;  /* query type */
  char *key; /* from strdup */

  char *data;  /* from cb  */
  size_t size; /* from cb  */

  CURL *hnd;
  struct curl_slist *hdrs;
};

static void
get_models_hnd (pack_t *pack)
{
  json_t *json = json_loadb (pack->data, pack->size, 0, NULL);

  if (json)
    {
      fprintf (models, "%s\n", pack->key);
      json_t *arr = json_object_get (json, "data");
      for (size_t n = json_array_size (arr), i = 0; i < n; i++)
        {
          json_t *item = json_array_get (arr, i);
          json_t *id = json_object_get (item, "id");
          fputs (json_string_value (id), models);
          fputc ('\n', models);
        }
      got_models = true;
      json_decref (json);
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
get_models (const char *key)
{
  CURL *hnd = curl_easy_init ();
  pack_t *pack = malloc (sizeof (pack_t));

#define URL_MODELS "https://api.siliconflow.cn/v1/models"

  curl_easy_setopt (hnd, CURLOPT_URL, URL_MODELS);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");

  static char hdr_key[80];
  sprintf (hdr_key, "authorization: Bearer %s", key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, hdr_key);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  char *dup = strdup (key);
  curl_easy_setopt (hnd, CURLOPT_PRIVATE, pack);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, pack);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_models_cb);

  curl_multi_add_handle (multi, hnd);

  pack->key = dup;
  pack->type = TYPE_MODELS;

  pack->size = 0;
  pack->data = NULL;

  pack->hnd = hnd;
  pack->hdrs = hdrs;
}

static void
get_info_hnd (pack_t *pack)
{
  json_t *json = json_loadb (pack->data, pack->size, 0, NULL);

  if (json)
    {
      json_t *obj = json_object_get (json, "data");
      const char *name = json_string_value (json_object_get (obj, "name"));
      const char *balance
          = json_string_value (json_object_get (obj, "balance"));
      fprintf (result, "%s, %s, %s\n", pack->key, name, balance);
      json_decref (json);
    }
  else
    fprintf (result, "%s, , \n", pack->key);
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
get_info (const char *key)
{
  CURL *hnd = curl_easy_init ();
  pack_t *pack = malloc (sizeof (pack_t));

#define URL_INFO "https://api.siliconflow.cn/v1/user/info"

  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt (hnd, CURLOPT_URL, URL_INFO);

  static char hdr_key[80];
  sprintf (hdr_key, "authorization: Bearer %s", key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, hdr_key);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  char *dup = strdup (key);
  curl_easy_setopt (hnd, CURLOPT_PRIVATE, pack);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, pack);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_info_cb);

  curl_multi_add_handle (multi, hnd);

  pack->key = dup;
  pack->type = TYPE_INFO;

  pack->size = 0;
  pack->data = NULL;

  pack->hnd = hnd;
  pack->hdrs = hdrs;
}

static void
work (const char *path)
{
  FILE *file = fopen (path, "r");
  assert (file);

  for (char key[64]; fgets (key, sizeof (key), file);)
    {
      if (key[0] == '\n')
        continue;

      key[strlen (key) - 1] = '\0';

      get_models (key);
      get_info (key);
    }

  for (int n = 1, r; n;)
    {
      CURLMcode code = curl_multi_perform (multi, &n);

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
              continue;

            if (pack->type == TYPE_MODELS && !got_models)
              get_models_hnd (pack);
            else if (pack->type == TYPE_INFO)
              get_info_hnd (pack);

            curl_slist_free_all (pack->hdrs);
            free (pack->data);
            free (pack->key);
            free (pack);

            curl_multi_remove_handle (multi, m->easy_handle);
            curl_easy_cleanup (hnd);
          }
    }
}
