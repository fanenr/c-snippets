#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>

#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

void
handle_request (struct evhttp_request *req, void *arg)
{
  struct evbuffer *out = evhttp_request_get_output_buffer (req);
  evbuffer_add_printf (out, "Hello World!");
  evhttp_send_reply (req, HTTP_OK, "OK", NULL);
}

int
main (void)
{
  struct evhttp *http;
  struct event_base *base;

  base = event_base_new ();
  http = evhttp_new (base);

  evhttp_bind_socket (http, "127.0.0.1", 3354);
  evhttp_set_gencb (http, handle_request, NULL);

  event_base_dispatch (base);
  event_base_free (base);
  evhttp_free (http);
}
