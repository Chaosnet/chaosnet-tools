#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chaos.h"

static const char *contact = "SHUTDOWN";

static void usage(char *name)
{
  fprintf(stderr, "Usage: %s <host> [data]\n", name);
  exit(1);
}

int main(int argc, char **argv)
{
  int fd;

  if (argc < 2 || argc > 3)
    usage(argv[0]);

  fd = chaos_stream();
  if (argc == 2)
    chaos_stream_rfc(fd, argv[1], contact);
  else
    chaos_stream_rfc_data(fd, argv[1], contact, argv[2], strlen(argv[2]));

  return 0;
}
