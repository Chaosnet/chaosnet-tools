#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "libword.h"
#include "mldev/mldev.h"

static word_t buffer[MAX_READ];

static void convert (word_t *file, const char *name)
{
  const char *p;
  word_t syllable;
  word_t c;
  int i;
  
  file[0] = 0446353000000LL; // DSK
  file[1] = 0;
  file[2] = 0360000000000LL; // >
  file[3] = 0166445556016LL; // .TEMP.

  syllable = 0;
  i = 30;
  for (p = name; ; p++) {
    c = *p;
    switch (c) {
    case ':':
      file[0] = syllable;
      syllable = 0;
      i = 30;
      break;
    case ';':
      file[3] = syllable;
      syllable = 0;
      i = 30;
      break;
    case 0:
    case ' ':
      if (syllable)
        file[file[1] == 0 ? 1 : 2] = syllable;
      syllable = 0;
      i = 30;
      if (c == 0)
        return;
      break;
    case 021:
      c = *++p;
      if (c == 0)
        return;
      /* Fall through. */
    default:
      if (i < 0)
        break;
      if (c >= 0140)
        syllable |= (c - 0100) << i;
      else
        syllable |= (c - 040) << i;
      i -= 6;
      break;
    }
  }
}

static int transfer_read(FILE *f)
{
  int m, n = MAX_READ;
  word_t *p = buffer;

  n = mldev_read(buffer, n);
  m = n;
  while (n-- > 0)
    write_word(f, *p++);
  return m;
}

static int transfer_write(FILE *f)
{
  int m, n = MAX_WRITE;
  word_t *p = buffer;

  for (n = 0; n < MAX_WRITE; n++) {
    word_t data = get_word(f);
    if (data == -1)
      break;
    *p++ = data;
  }
  if (n == 0)
    return 0;
  m = mldev_write(buffer, n);
  if (m != n)
    exit(1);
  return n;
}

static void usage(char *x)
{
  fprintf(stderr, "Usage: %s -r|-w [-W<word format>] <host> <ITS file> <local file>\n\n", x);
  usage_word_format();
  exit(1);
}

int main(int argc, char **argv)
{
  int (*transfer)(FILE *f) = NULL;
  FILE *f = NULL;
  const char *remote_file = NULL;
  const char *local_file = NULL;
  char *host = NULL;
  word_t its_file[4];
  int flags;
  int n, x;
  int opt;

  input_word_format = &its_word_format;
  output_word_format = &its_word_format;

  while ((opt = getopt(argc, argv, "rwW:")) != -1) {
    switch (opt) {
    case 'r':
      flags = O_RDONLY;
      transfer = transfer_read;
      break;
    case 'w':
      flags = O_WRONLY;
      transfer = transfer_write;
      break;
    case 'W':
      if (parse_input_word_format(optarg))
        usage(argv[0]);
      if (parse_output_word_format(optarg))
        usage(argv[0]);
      break;
    default:
      usage(argv[0]);
    }
  }

  if (transfer == NULL)
    usage(argv[0]);

  if (argc - optind != 3)
    usage(argv[0]);

  host = argv[optind];
  remote_file = argv[optind + 1];
  local_file = argv[optind + 2];

  if (flags == O_WRONLY) {
    f = fopen(local_file, "rb");
    if (f == NULL) {
      fprintf(stderr, "Error opening local file %s: %s\n",
              local_file, strerror(errno));
      exit(1);
    }
  }

  mldev_init(host);
  convert (its_file, remote_file);
  x = mldev_open(its_file, flags);
  if (x < 0) {
    fprintf(stderr, "Error opening remote file: %s\n", strerror(-x));
    exit(1);
  }

  if (flags == O_RDONLY) {
    f = fopen(local_file, "wb");
    if (f == NULL) {
      fprintf(stderr, "Error opening local file %s: %s\n",
              local_file, strerror(errno));
      exit(1);
    }
  }

  do {
    n = transfer(f);
    if (n < 0) {
      fprintf(stderr, "Error transferring file.\n");
      exit(1);
    }
  } while (n > 0);

  flush_word(f);
  fclose(f);
  mldev_close();
  return 0;
}
