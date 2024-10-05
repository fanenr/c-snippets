#include <event2/event.h>
#include <stdbool.h>
#include <stdio.h>

static int loops;
static struct timeval tv = { .tv_sec = 1 };

void
timer_cb (evutil_socket_t fd, short flags, void *arg)
{
  struct event *ev = arg;
  printf ("loops: %d\n", loops);

  if (loops == 2)
    {
      event_del (ev);
      event_assign (ev, event_get_base (ev), fd, flags & ~EV_PERSIST,
                    event_get_callback (ev), event_get_callback_arg (ev));
      event_add (ev, &tv);
    }

  if (loops >= 5)
    event_free (ev);
}

int
main (void)
{
  struct event_base *base = event_base_new ();
  event_base_priority_init (base, 1);

  struct event *ev = event_new (base, -1, EV_TIMEOUT | EV_PERSIST, timer_cb,
                                event_self_cbarg ());

  event_add (ev, &tv);

  for (; loops < 6; loops++)
    event_base_loop (base, EVLOOP_ONCE);

  if (loops >= 6)
    event_free (ev);

  event_base_free (base);
}
