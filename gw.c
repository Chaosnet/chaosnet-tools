#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "chaos.h"

static const char *argv0;
static const char *port;
static const char *contact;
static const char *host;
static char *peer;
static int server;
static char buffer[MAX_PACKET];

static void fatal(const char *message, int error)
{
  fprintf(stderr, "Fatal error %s\n", message);
  if (error)
    fprintf(stderr, ": %s", strerror(error));
  fputc('\n', stderr);
  exit(1);
}

static void disconnect(const char *message, int error, int code)
{
  fprintf(stderr, "Host %s %s\n", peer, message);
  if (error)
    fprintf(stderr, ": %s", strerror(error));
  fputc('\n', stderr);
  exit(code);
}

static void copy(int src, int dst)
{
  ssize_t n = read(src, buffer, sizeof buffer);
  if (n < 0)
    disconnect("read error", errno, 1);
  if (n == 0)
    disconnect("connection closed", 0, 0);
  if (write(dst, buffer, n) < 0)
    disconnect("write error", errno, 1);
}

static void forward(int s, int c)
{
  struct pollfd pfd[2];

  pfd[0].fd = s;
  pfd[0].events = POLLIN|POLLERR|POLLHUP;
  pfd[1].fd = c;
  pfd[1].events = POLLIN|POLLERR|POLLHUP;

  for(;;) {
    int x = poll(pfd, 2, 10*1000);
    if (x < 0) {
      fprintf(stderr, "Poll error: %s\n", strerror(errno));
      continue;
    }
    if (x == 0)
      continue;
    if (pfd[0].revents & POLLIN)
      copy(pfd[0].fd, pfd[1].fd);
    if (pfd[1].revents & POLLIN)
      copy(pfd[1].fd, pfd[0].fd);
    if ((pfd[0].revents | pfd[1].revents) & POLLERR)
      disconnect("connection error", 0, 1);
    if ((pfd[0].revents | pfd[1].revents) & POLLHUP)
      disconnect("connection hung up", 0, 1);
  }
}

static void incoming(void)
{
  struct sockaddr addr;
  socklen_t len = sizeof addr;
  int s, c;

  s = accept(server, &addr, &len);
  if (s < 0) {
    fprintf(stderr, "Error calling accept: %s\n", strerror(errno));
    return;
  }

  if (fork()) {
    close(s);
    return;
  }

  if (addr.sa_family == AF_INET) {
    struct sockaddr_in *x = (struct sockaddr_in *)&addr;
    peer = inet_ntoa(x->sin_addr);
    fprintf(stderr, "Incoming connection from %s to port %s\n", peer, port);
  }

  c = chaos_stream();
  if (c < 0)
    fatal("creating Chaosnet stream", errno);

  if (chaos_rfc(c, host, contact) < 0)
    fatal("error during request for connection", errno);
  fprintf(stderr, "Opened connection to %s contact %s\n", host, contact);
  forward(s, c);
}

static void serve(void)
{
  struct addrinfo hints;
  struct addrinfo *addr;
  struct addrinfo *rp;
  int reuse = 1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, port, &hints, &addr) != 0)
    fatal("calling getaddrinfo", errno);

  for (rp = addr; rp != NULL; rp = rp->ai_next) {
    server = socket(rp->ai_family, rp->ai_socktype, 0);
    if (server < 0)
      fatal("creating socket", errno);

    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) < 0)
      fatal("calling setsockopt", errno);

    if (bind(server, rp->ai_addr, rp->ai_addrlen) == 0)
      goto ok;
    fprintf(stderr, "Bind: %s\n", strerror(errno));
  }
  fatal("binding socket", 0);

 ok:
  freeaddrinfo(addr);

  if (listen(server, 1) < 0)
    fatal("listening", errno);

  for (;;)
    incoming();
}

static int usage(int code)
{
  FILE *f = code ? stderr : stdout;
  fprintf(f, "Usage: %s [-h] <port> <contact> <host>\n", argv0);
  exit(code);
}

int main(int argc, char **argv)
{
  int c;

  argv0 = argv[0];

  while((c = getopt(argc, argv, "h")) != -1) {
    switch(c) {
    case 'h':
      usage(0);
      break;
    default:
      usage(1);
      break;
    }
  }

  if (argc - optind != 3)
    usage(1);

  port = argv[optind];
  contact = argv[optind + 1];
  host = argv[optind + 2];

  serve();

  return 0;
}
