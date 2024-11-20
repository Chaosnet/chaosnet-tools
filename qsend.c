/* Copyright © 2024 Lars Brinkoff <lars@nocrew.org> */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "chaos.h"

#define END_OF_LINE  0215  /* Lispm end of line character. */

static const char *contact = "SEND";

static char *hostname(void)
{
  static char buf[100];
  FILE *f = popen("hostname", "r");
  size_t n;

  if (f == NULL)
    return "(unknown)";

  n = fread(buf, 1, sizeof buf, f);
  buf[n] = 0;
  while(buf[--n] == '\n')
    buf[n] = 0;

  pclose(f);

  return buf;
}

static void header(int fd, char *sender, char *timestamp)
{
  char buf[1024], tbuf[100];
  char *atsign = "";
  char *host = "";
  size_t n;

  if (sender == NULL) {
    sender = getenv("USER");
    atsign = "@";
    host = hostname();
  }

  if (timestamp == NULL) {
    time_t now = time(NULL);
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %T", localtime(&now));
    timestamp = tbuf;
  }

  n = snprintf(buf, sizeof buf, "%s%s%s %s%c",
               sender, atsign, host, timestamp, END_OF_LINE);
  write(fd, buf, n);
}

static void usage(char *name)
{
  fprintf(stderr, "Usage: %s <user>@<host> [<sender>@<host> [<time>]]\n",
          name);
  exit(1);
}

int main(int argc, char **argv)
{
  char *user, *host;
  char *sender = NULL;
  char *timestamp = NULL;
  int fd;

  if (argc < 2 && argc > 4)
    usage(argv[0]);

  user = strdup(argv[1]);
  host = strchr(user, '@');
  if (host == NULL) {
    fprintf(stderr, "No destination host specified.\n");
    usage(argv[0]);
  }
  *host++ = 0;

  if (argc > 2)
    sender = argv[2];

  if (argc > 3)
    timestamp = argv[3];

  fd = chaos_stream();
  chaos_stream_rfc_data(fd, host, contact, user, strlen(user));

  header(fd, sender, timestamp);

  for(;;) {
    int c = getchar();
    char data;
    if (c == EOF)
      exit(0);
    else if (c == '\n')
      data = END_OF_LINE;
    else
      data = c;
    if (write(fd, &data, 1) != 1)
      exit(1);
  }

  return 0;
}
