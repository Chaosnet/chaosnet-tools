/* Copyright © 2020 Björn Victor (bjorn@victor.se) */
/* Copyright © 2023, 2024 Lars Brinkoff <lars@nocrew.org> */

/* SEND server. */

/*
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/errno.h>

#include "chaos.h"

// default window size
static int winsize = 15;

static int daemonize = 0;
static char peer[MAX_PACKET];

static const char *contact = "SEND";
static const char *qsend_handler;
static FILE *qsend_file;

static void serve(void);
static void send_packet(int opcode, const void *data, size_t len);
static void close_connection(const char *message);
static void handle_packet(void);

typedef void handler_t(const unsigned char *data, int len);

static handler_t packet_rfc;
static handler_t packet_los;
static handler_t packet_cls;
static handler_t packet_dat;

struct handler {
  int opcode;
  handler_t *fn;
};

struct handler packet_handler[] = {
  { CHOP_RFC, packet_rfc },
  { CHOP_LOS, packet_los },
  { CHOP_CLS, packet_cls },
  { CHOP_DAT, packet_dat }
};

#define MAX_HANDLERS (sizeof packet_handler / sizeof packet_handler[0])

static FILE *log, *debug;
static int sock = -1;

static void dispatch(int opcode, int n, struct handler *handler,
                     const unsigned char *data, int len)
{
  int i;
  for (i = 0; i < n; i++) {
    if (opcode == handler[i].opcode) {
      handler[i].fn(data, len);
      return;
    }
  }
  fprintf(stderr, "Peer %s: Unknown operation: %d\n", peer, opcode);
  exit(1);
}

static void close_connection(const char *message)
{
  pclose(qsend_file);
  qsend_file = NULL;

  char tbuf[128];
  time_t now = time(NULL);
  strftime(tbuf, sizeof(tbuf), "%T", localtime(&now));

  fprintf(log, "%s: Closing peer %s: %s\n", tbuf, peer, message);
  send_packet(CHOP_CLS, message, strlen(message));
  if (*peer)
    exit(0);
  else
    serve();

}

static void send_packet(int opcode, const void *data, size_t len)
{
  if (chaos_packet_send(sock, opcode, data, len) < 0)
    close_connection("Network send error");
}

static void packet_rfc(const unsigned char *data, int len)
{
  char command[1024];
  char *user;

  if (fork())
    serve();
  else {
    char tbuf[128];
    time_t now = time(NULL);
    strftime(tbuf, sizeof(tbuf), "%T", localtime(&now));
    strncpy(peer, (const char *)data, len);

    user = strchr(peer, ' ');
    if (user)
      *user++ = 0;
    else
      user = "";

    fprintf(log, "%s: Open connection from %s\n", tbuf, peer);
    send_packet(CHOP_OPN, NULL, 0);

    snprintf(command, sizeof command, "\'%s\' \'%s\' \'%s'",
             qsend_handler, user, peer);
    qsend_file = popen(command, "w");
    if (qsend_file == NULL)
      close_connection("Can't deliver message");
    fprintf(log, "%s: Delivered message from %s to %s\n", tbuf, peer, user);
  }
}

static void packet_los(const unsigned char *data, int len)
{
  char buf[MAX_PACKET+1];
  if (len > MAX_PACKET)
    len = MAX_PACKET;
  strcpy(buf, "Error from peer: ");
  strncpy(buf + strlen(buf), (const char *)data, (size_t)len);
  close_connection(buf);
}

static void packet_cls(const unsigned char *data, int len)
{
  char buf[MAX_PACKET];
  if (len > MAX_PACKET)
    len = MAX_PACKET;
  strcpy(buf, "Peer closed connection: ");
  strncpy(buf + strlen(buf), (const char *)data, (size_t)len);
  close_connection(buf);
}

static void packet_dat(const unsigned char *data, int len)
{
  int i;
  for (i = 0; i < len; i++) {
    if (data[i] == 0215)
      fputc('\n', qsend_file);
    else
      fputc(data[i], qsend_file);
  }
}

static void
handle_packet(void) {
  unsigned char buf[MAX_PACKET];
  int opcode;
  ssize_t n = chaos_packet_recv(sock, &opcode, buf);
  if (n == -1)
    close_connection("Connection error");
  else if (n == 0)
    close_connection("Peer closed connection");
  else
    dispatch(opcode, MAX_HANDLERS, packet_handler, buf, n);
}

static void serve(void)
{
  close(sock);
  *peer = 0;
  sock = chaos_packets();
  if (sock < 0) {
    fprintf(stderr, "Error connecting to Chaosnet packet NCP.\n");
    exit(1);
  }
  char cwa[MAX_PACKET];
  int n = sprintf(cwa, "[winsize=%d] %s", winsize, contact);
  send_packet(CHOP_LSN, cwa, n);
}  

static void usage(char *s)
{
  fprintf(stderr, "Usage: %s [-dqv]\n", s);
  fprintf(stderr, "  -d    Run as daemon.\n");
  fprintf(stderr, "  -q    Quiet operation - no logging, just errors.\n");
  fprintf(stderr, "  -v    Verbose operation - detailed logging.\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  char *pname;
  int quiet = 0, verbose = 0;
  int c;

  qsend_handler = getenv("QSEND");
  if (qsend_handler == NULL)
    qsend_handler = "qsend-incoming";

  pname = argv[0];
  log = stderr;
  debug = stderr;

  while ((c = getopt(argc, argv, "dqv")) != -1) {
    switch (c) {
    case 'd':
      daemonize = 1;
      break;
    case 'q':
      quiet = 1;
      break;
    case 'v':
      verbose++;
      break;
    default:
      fprintf(stderr, "Unknown option: %c\n", c);
      usage(pname);
    }
  }
  argc -= optind;
  argv += optind;

  if (argc > 0)
    usage(pname);

  if (quiet)
    log = fopen("/dev/null", "w");

  if (!verbose)
    debug = fopen("/dev/null", "w");

  if (daemonize) {
#ifdef __APPLE__
    fprintf(stderr, "Daemon on supported on Mac.\n");
    exit(1);
#else
    daemon(1, 1);
#endif
  }

  serve();
  for (;;)
    handle_packet();
}
