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

  remove ("./result");
  result = fopen ("./result", "a");
  assert (result);

  fputs ("key, name, balance\n", result);
}

static void
cleanup (void)
{
  curl_global_cleanup ();
  fclose (result);
}

static size_t
get_models_cb (char *data, size_t size, size_t nmemb, void *ptr)
{
  size_t total = size * nmemb;
  bool *ret = ptr;

  FILE *file = fopen ("./models", "w");
  assert (file);

  json_t *json = json_loadb (data, total, 0, NULL);

  if (json)
    {
      json_t *arr = json_object_get (json, "data");
      for (size_t n = json_array_size (arr), i = 0; i < n; i++)
        {
          json_t *item = json_array_get (arr, i);
          json_t *id = json_object_get (item, "id");
          fputs (json_string_value (id), file);
          fputc ('\n', file);
        }
      *ret = true;
    }
  else
    *ret = false;

  json_decref (json);
  fclose (file);
  return total;
}

static bool
get_models (const char *key)
{
  bool ret;
  CURL *hnd = curl_easy_init ();

  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt (hnd, CURLOPT_URL, "https://api.siliconflow.cn/v1/models");

  static char key_hdr[78] = "authorization: Bearer ";
  strcpy (key_hdr + 22, key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, key_hdr);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, &ret);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_models_cb);

  curl_easy_perform (hnd);
  curl_easy_cleanup (hnd);
  return ret;
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

  json_decref (json);
  return total;
}

static void
get_info (const char *key)
{
  CURL *hnd = curl_easy_init ();

  curl_easy_setopt (hnd, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt (hnd, CURLOPT_URL,
                    "https://api.siliconflow.cn/v1/user/info");

  static char key_hdr[78] = "authorization: Bearer ";
  strcpy (key_hdr + 22, key);

  struct curl_slist *hdrs = NULL;
  hdrs = curl_slist_append (hdrs, key_hdr);
  hdrs = curl_slist_append (hdrs, "accept: application/json");

  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, key);
  curl_easy_setopt (hnd, CURLOPT_HTTPHEADER, hdrs);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, get_info_cb);

  CURLcode ret = curl_easy_perform (hnd);
  curl_easy_cleanup (hnd);
}

static void
work (const char *path)
{
  FILE *file = fopen (path, "r");
  assert (file);

  bool got_models = false;

  for (char key[64]; fgets (key, sizeof (key), file);)
    {
      if (key[0] == '\n')
        continue;

      key[strlen (key) - 1] = '\0';

      if (!got_models)
        got_models = get_models (key);

      get_info (key);
    }
}
