#include <event2/event.h>
#include <stdio.h>

static FILE *logfile;

static void
log_discard_cb (int severity, const char *msg)
{
}

static void
log_file_cb (int severity, const char *msg)
{
  static const char *prefixes[] = {
    [EVENT_LOG_DEBUG] = "debug",
    [EVENT_LOG_MSG] = "message",
    [EVENT_LOG_WARN] = "warn",
    [EVENT_LOG_ERR] = "error",
  };
  fprintf (logfile, "[%s] %s\n", prefixes[severity], msg);
}

void
log_discard (void)
{
  event_set_log_callback (log_discard_cb);
}

void
log_file (FILE *file)
{
  logfile = file;
  event_set_log_callback (log_file_cb);
}

int
main (void)
{
  libevent_global_shutdown ();
}
