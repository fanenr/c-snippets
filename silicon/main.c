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
}

static size_t
get_models_cb (char *data, size_t size, size_t nmemb, void *ptr)
{
  size_t total = size * nmemb;

  if (got_models)
    return total;

  json_t *json = json_loadb (data, total, 0, NULL);

  if (json)
    {
      fprintf (models, "%s\n", (const char *)ptr);
      json_t *arr = json_object_get (json, "data");
      for (size_t n = json_array_size (arr), i = 0; i < n; i++)
        {
          json_t *item = json_array_get (arr, i);
          json_t *id = json_object_get (item, "id");
          fputs (json_string_value (id), models);
          fputc ('\n', models);
        }
      got_models = true;
    }

  return total;
}

static void
get_models (const char *key)
{
  CURL *hnd = curl_easy_init ();

#define URL_MODELS "https://api.siliconflow.cn/v1/models"

  curl_easy_setopt (hnd, CURLOPT_URL, URL_MODELS);
  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");

  static char hdr_key[80];
  sprintf (hdr_key, "authorization: Bearer %s", key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, hdr_key);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  char *dup = strdup (key);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, dup);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_models_cb);

  curl_multi_add_handle (multi, hnd);
}

static size_t
get_info_cb (char *data, size_t size, size_t nmemb, void *ptr)
{
  size_t total = size * nmemb;
  json_t *json = json_loadb (data, total, 0, NULL);

  if (json)
    {
      json_t *obj = json_object_get (json, "data");
      const char *name = json_string_value (json_object_get (obj, "name"));
      const char *balance
          = json_string_value (json_object_get (obj, "balance"));
      fprintf (result, "%s, %s, %s\n", (const char *)ptr, name, balance);
    }
  else
    fprintf (result, "%s, , \n", (const char *)ptr);

  return total;
}

static void
get_info (const char *key)
{
  CURL *hnd = curl_easy_init ();

#define URL_INFO "https://api.siliconflow.cn/v1/user/info"

  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt (hnd, CURLOPT_URL, URL_INFO);

  static char hdr_key[80];
  sprintf (hdr_key, "authorization: Bearer %s", key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, hdr_key);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  char *dup = strdup (key);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, dup);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_info_cb);

  curl_multi_add_handle (multi, hnd);
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

  for (int run = 1; run;)
    {
      CURLMcode code = curl_multi_perform (multi, &run);
      assert (code == CURLM_OK);
      curl_multi_poll (multi, NULL, 0, 1000, NULL);

      int msgq;
      struct CURLMsg *msg;

      do
        {
          if (!(msg = curl_multi_info_read (multi, &msgq)))
            break;

          if (msg->msg != CURLMSG_DONE)
            continue;

          curl_multi_remove_handle (multi, msg->easy_handle);
        }
      while (msg);
    }
}
