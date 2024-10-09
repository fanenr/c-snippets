#include <event2/event.h>
#include <stdio.h>

static int loops;
static struct timeval tv = { .tv_sec = 1 };

void
timer_cb (evutil_socket_t fd, short events, void *arg)
{
  struct event *ev = arg;
  printf ("loops: %d\n", loops);

  if (loops == 2)
    {
      event_del (ev);
      event_assign (ev, event_get_base (ev), fd, events & ~EV_PERSIST,
                    event_get_callback (ev), event_get_callback_arg (ev));
      event_add (ev, &tv);
    }
}

int
main (void)
{
  struct event *ev;
  struct event_base *base;

  base = event_base_new ();
  event_base_priority_init (base, 1);

  ev = event_new (base, -1, EV_TIMEOUT | EV_PERSIST, timer_cb,
                  event_self_cbarg ());
  event_add (ev, &tv);

  for (; loops < 6; loops++)
    event_base_loop (base, EVLOOP_ONCE);

  event_free (ev);
  event_base_free (base);
}
