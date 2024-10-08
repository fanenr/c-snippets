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
  struct evbuffer *out = bufferevent_get_output (bev);
  struct evbuffer *in = bufferevent_get_input (bev);
  evbuffer_add_buffer (out, in);
}

static void
write_cb (struct bufferevent *bev, void *arg)
{
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
accept_cb (struct evconnlistener *lev, int clnt, struct sockaddr *addr,
           int len, void *arg)
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
  struct event_base *base;
  struct evconnlistener *lev;

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons (3354),
    .sin_addr.s_addr = htonl (INADDR_ANY),
  };

  base = event_base_new ();

  if (-1 == (serv = socket (AF_INET, SOCK_STREAM, 0)))
    abort ();
  if (0 != bind (serv, (void *)&addr, sizeof (addr)))
    abort ();
  if (0 != listen (serv, 32))
    abort ();

  evutil_make_socket_nonblocking (serv);
  evutil_make_listen_socket_reuseable (serv);

  lev = evconnlistener_new (base, NULL, NULL, 0, 0, serv);
  evconnlistener_set_error_cb (lev, accept_error_cb);
  evconnlistener_set_cb (lev, accept_cb, NULL);

  event_base_dispatch (base);
  event_base_free (base);
  close (serv);
}
