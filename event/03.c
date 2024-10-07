#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>

#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void
read_cb (struct bufferevent *bev, void *arg)
{
  struct evbuffer *in = bufferevent_get_input (bev);
  struct evbuffer *out = bufferevent_get_output (bev);

  evbuffer_add_buffer (out, in);
}

static void
write_cb (struct bufferevent *bev, void *arg)
{
  struct evbuffer *out = bufferevent_get_output (bev);
  struct evbuffer *in = bufferevent_get_input (bev);
  evbuffer_add_buffer (out, in);
}

static void
event_cb (struct bufferevent *bev, short events, void *arg)
{
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
      bufferevent_free (bev);
      close (bufferevent_getfd (bev));
    }
}

void
accept_cb (struct evconnlistener *lev, evutil_socket_t clnt,
           struct sockaddr *addr, int len, void *arg)
{
  struct event_base *base = evconnlistener_get_base (lev);
  struct bufferevent *bev = bufferevent_socket_new (base, clnt, 0);
  bufferevent_setcb (bev, read_cb, write_cb, event_cb, NULL);
  bufferevent_enable (bev, EV_READ | EV_WRITE);
}

static void
accept_error_cb (struct evconnlistener *lev, void *ctx)
{
  struct event_base *base = evconnlistener_get_base (lev);
  event_base_loopexit (base, NULL);
}

int
main (void)
{
  int serv;

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons (3354),
    .sin_addr.s_addr = htonl (INADDR_ANY),
  };

  if (-1 == (serv = socket (AF_INET, SOCK_STREAM, 0)))
    abort ();
  if (0 != bind (serv, (void *)&addr, sizeof (addr)))
    abort ();
  if (0 != listen (serv, 32))
    abort ();

  struct event_base *base = event_base_new ();
  struct evconnlistener *lev
      = evconnlistener_new (base, accept_cb, NULL, LEV_OPT_REUSEABLE, 0, serv);

  evconnlistener_set_error_cb (lev, accept_error_cb);

  event_base_dispatch (base);
  event_base_free (base);
  close (serv);
}
