#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "../chaos.h"

static int fd;

static void fatal(const char *message, int error)
{
  fprintf(stderr, "Fatal error %s\n", message);
  if (error)
    fprintf(stderr, ": %s", strerror(error));
  fputc('\n', stderr);
  exit(1);
}

void io_init (const char *host, const char *port)
{
  fd = chaos_stream();
  if (fd < 0)
    fatal ("creating Chaosnet stream\n", errno);
  if (chaos_stream_rfc(fd, host, port) < 0)
    fatal("during request for connection", errno);
  fprintf(stderr, "Opened connection to Chaosnet host %s, contact %s\n",
          host, port);
}

void io_flush (void)
{
}

void io_write (void *data, int n)
{
  ssize_t m = write (fd, data, n);
  if (m < 0)
    fatal ("writing to Chaosnet", errno);
  else if (m != n)
    fatal ("writing to Chaosnet", 0);
}

void io_read (void *data, int n)
{
  int m;
  while (n > 0)
    {
      m = read (fd, data, n);
      if (m <= 0)
        fatal ("reading from Chaosnet", errno);
      data += m;
      n -= m;
    }
}


