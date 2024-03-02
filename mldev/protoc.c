#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "io.h"
#include "protoc.h"

static word_t ascii_to_sixbit (char *ascii)
{
  char *spaces = "      ";
  word_t word = 0;
  int i;

  for (i = 0; i < 6; i++)
    {
      word <<= 6;
      if (*ascii == 0)
	ascii = spaces;
      word += ((*ascii++) - 040) & 077;
    }

  return word;
}

static void send_words (word_t word1, word_t word2)
{
  unsigned char data[9];

  data[0] = (word1 >> 28) & 0xff;
  data[1] = (word1 >> 20) & 0xff;
  data[2] = (word1 >> 12) & 0xff;
  data[3] = (word1 >>  4) & 0xff;
  data[4] = ((word1 <<  4) & 0xf0) + ((word2 >> 32) & 0x0f);
  data[5] = (word2 >> 24) & 0xff;
  data[6] = (word2 >> 16) & 0xff;
  data[7] = (word2 >>  8) & 0xff;
  data[8] = (word2 >>  0) & 0xff;

  io_write (data, 9);
}

void recv_words (word_t *word1, word_t *word2)
{
  unsigned char data[9];
  word_t x;

  io_read (data, 9);

  x = data[0];
  x = (x << 8) + data[1];
  x = (x << 8) + data[2];
  x = (x << 8) + data[3];
  *word1 = (x << 4) + (data[4] >> 4);

  x = data[4] & 0x0f;
  x = (x << 8) + data[5];
  x = (x << 8) + data[6];
  x = (x << 8) + data[7];
  *word2 = (x << 8) + data[8];
}

static int file_eof;
static int file_error;

static int request (int cmd, int n, word_t *args, word_t *reply)
{
  word_t aobjn;
  int i;

  file_eof = 0;
  file_error = 0;

  aobjn = (-n << 18) + cmd;
  send_words (aobjn, args[0]);
  for (i = 1; i < n; i += 2)
    send_words (args[i], args[i+1]);
  io_flush ();

  if (reply == NULL)
    return 0;

 again:
  recv_words (&aobjn, &reply[0]);
  n = (aobjn >> 18);
  if (n != 0)
    n = 01000000 - n;
  for (i = 1; i < n; i += 2)
    recv_words (&reply[i], &reply[i+1]);

  switch (aobjn & 0777777LL)
    {
    case RDATA:
      break;
    case ROPENI:
      if (reply[0] == 0777777777777LL)
	{
	  if (reply[5] != 0777777777777LL)
	    fprintf (stderr, "   %lld words, %lld %lld-bit bytes\n",
		     reply[7], reply[5], reply[6]);
	  if (reply[9] == 0777777777777LL)
	    {
	      //print_datime (stderr, reply[10]);
	      ;
	    }
	}
      else
	{
	  fprintf (stderr, "   Error %llo\n", reply[0]);
	  file_error = reply[0];
	}
      break;
    case ROPENO:
      if (reply[0] == 0777777777777LL)
	{
	  if (reply[5] != 0777777777777LL)
	    fprintf (stderr, "   %lld words, %lld %lld-bit bytes\n",
		     reply[7], reply[5], reply[6]);
	}
      else
	{
	  fprintf (stderr, "   Error %llo\n", reply[0]);
	  file_error = reply[0];
	}
      break;
    case REOF:
      file_eof = 1;
      if (cmd != CALLOC)
	goto again;
      break;
    case RNOOP:
      break;
    case RICLOS:
      break;
    case ROCLOS:
      break;
    case RIOC:
      file_error = reply[0];
      break;
    default:
      fprintf (stderr, "Unknown reply: %012llo\n", aobjn);
      exit (1);
    }

  return n;
}

int protoc_open (const word_t *file, int mode)
{
  word_t args[5];
  word_t reply[11];
  int cmd;
  int n;

  if ((mode & 1) == 0)
    cmd = COPENI;
  else
    cmd = COPENO;

  args[0] = file[0];
  args[1] = file[1];
  args[2] = file[2];
  args[3] = file[3];
  args[4] = mode;
  n = request (cmd, 5, args, reply);
  if (file_error)
    return -file_error;

  return n;
}

int protoc_iclose (void)
{
  word_t args[1];
  word_t reply[1];

  args[0] = 0;
  return request (CICLOS, 1, args, reply);
}

int protoc_oclose (void)
{
  word_t args[1];
  word_t reply[1];

  args[0] = 0;
  return request (COCLOS, 1, args, reply);
}

int protoc_read (word_t *buffer, int size)
{
  word_t args[1];
  int n;

  args[0] = 5 * size;
  n = request (CALLOC, 1, args, buffer);
  if (file_eof)
    return 0;
  else
    return n;
}

int protoc_write (const word_t *args, int n)
{
  n = request (CDATA, n, args, NULL);
  return 0;
}

int protoc_init (const char *host)
{
  io_init (host, "MLDEV");
  return 0;
}
