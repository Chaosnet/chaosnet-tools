#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "protoc.h"
#include "mldev.h"

static word_t buffer[MAX_READ + 1];

static void sixbit_to_ascii (word_t word, char *ascii)
{
  int i;

  for (i = 0; i < 6; i++)
    ascii[i] = 040 + ((word >> (6 * (5 - i))) & 077);
  ascii[6] = 0;
  for (i = 5; i > 0; i--) {
    if (ascii[i] == ' ')
      ascii[i] = 0;
    else
      return;
  }
}

static void word_to_ascii (word_t word, char *ascii)
{
  int i;
  for (i = 0; i < 5; i++)
    {
      *ascii++ = (word >> 29) & 0177;
      word <<= 7;
    }
  *ascii = 0;
}

void words_to_ascii (void *data, int n, char *ascii)
{
  word_t *word = data;
  int i;
  for (i = 0; i < n; i++)
    {
      word_to_ascii (*word++, ascii);
      ascii += 5;
    }
}

void ascii_to_words (void *data, int n, const char *ascii)
{
  word_t x, *word = data;
  int i, j;
  for (i = 0; i < n; i++)
    {
      x = 0;
      for (j = 0; j < 5; j++)
	{
	  x = (x << 7) + (*ascii++ & 0177);
	}
      *word++ = (x << 1);
    }
}

static char *unslash (char *name)
{
  char *p = name;
  while (*p)
    {
      if (*p == '/')
	*p = '|';
      p++;
    }
  return name;
}

static char *trim (char *name)
{
  char *p = name + 5;
  while ((*p == ' ' || *p == 0) && p >= name)
    {
      *p = 0;
      p--;
    }
  for (p = name; *p; ++p)
    {
      *p = toupper (*p);
    }
  return name;
}

static void print_date (FILE *f, word_t t)
{
  /* Bits 3.1-3.5 are the day, bits 3.6-3.9 are the month, and bits
     4.1-4.7 are the year. */

  int date = (t >> 18);
  int day = (date & 037);
  int month = (date & 0740);
  int year = (date & 0777000);

  fprintf (f, "%u-%02u-%02u", (year >> 9) + 1900, (month >> 5), day);

  if (year & 0600000)
    printf (" [WARNING: overflowed year field]");
}

static void print_datime (FILE *f, word_t t)
{
  /* The right half of this word is the time of day since midnight in
     half-seconds. */

  int seconds = (t & 0777777) / 2;
  int minutes = (seconds / 60);
  int hours = (minutes / 60);

  print_date (f, t);
  fprintf (f, " %02u:%02u:%02u", hours, (minutes % 60), (seconds % 60));
}

static void print_ascii (FILE *f, int n, word_t *words)
{
  char string[6];
  word_t word;

  while (n--)
    {
      word = *words++;
      word_to_ascii (word, string);
      fputs (string, f);
    }
}

static int unix_errno (int n)
{
  switch (n)
    {
    case -ENSDR:
    case -ENSFL: return -ENOENT;
    default:     return -EIO;
    }
}

/**********************************************************************/ 

static int current_flags = 0;

static void split_path (const char *path, char *device, char *sname,
			char *fn1, char *fn2)
{
  const char *start, *end;

  strcpy (device, "DSK   ");
  memset (sname, 0, 7);
  memset (fn1, 0, 7);
  memset (fn2, 0, 7);

  start = path;
  end = strchr (start, ':');
  if (end)
    {
      strncpy (device, start, end - start);
      start = end + 1;
    }
  else
    {
      strcpy (device, "DSK");
    }

  end = strchr (start, '/');
  if (end)
    {
      strncpy (sname, start, end - start);
      start = end + 1;
    }
  else
    {
      strcpy (sname, ".temp.");
    }

  end = strchr (start, '.');
  if (end)
    {
      strncpy (fn1, start, end - start);
      strcpy (fn2, end + 1);
    }
  else
    {
      strcpy (fn1, start);
      strcpy (fn2, ">");
    }

  trim (device);
  trim (fn1);
  trim (fn2);
  trim (sname);
}

int mldev_getattr(const char *path, struct mldev_file *file)
{
  split_path (path, file->device, file->sname, file->fn1, file->fn2);

  if (file->sname[0] == 0 || file->fn1[0] == 0)
    file->pack = MLDEV_DIR;
  else
    file->pack = 0;

  return 0;
}

int mldev_open (const word_t *file, int flags)
{
  int mode;
  int n;

  switch (flags)
    {
    case O_RDONLY: mode = 0; break;
    case O_WRONLY: mode = 1; break;
    default:       return -EACCES;
    }

  char x[7];
  sixbit_to_ascii(file[0], x);
  fprintf(stderr, "Open file:   %s:", x);
  sixbit_to_ascii(file[3], x);
  fprintf(stderr, "%s;", x);
  sixbit_to_ascii(file[1], x);
  fprintf(stderr, "%s ", x);
  sixbit_to_ascii(file[2], x);
  fprintf(stderr, "%s", x);

  n = protoc_open (file, mode);
  if (n < 0)
    return unix_errno (n);

  return 0;
}

int mldev_close(void)
{
  switch (current_flags)
    {
    case O_RDONLY: protoc_iclose(); break;
    case O_WRONLY: protoc_oclose(); break;
    default: return -EIO;
    }

  current_flags = 0;
  return 0;
}

int mldev_write(const word_t *buf, int size)
{
  int n;

  if (size == 0)
    return 0;

  if (size > MAX_WRITE)
    size = MAX_WRITE;

  buffer[0] = 5 * size;
  memcpy(buffer + 1, buf, size * sizeof(word_t));
  n = protoc_write (buffer, size + 1);
  if (n < 0)
    return -EIO;

  return size;
}

int mldev_read (word_t *buf, int size)
{
  int n;

  n = protoc_read (buffer, size + 1);
  if (n == 0)
    return 0;
  else if (n < 0)
    return -EIO;

  n = (buffer[0] + 4) / 5;
  if (n > MAX_READ)
    n = MAX_READ;
  memcpy(buf, buffer + 1, n * sizeof(word_t));

  return n;
}

void mldev_init (const char *host)
{
  protoc_init (host);
}
