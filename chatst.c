/* Copyright © 2023 Lars Brinkoff <lars@nocrew.org> */

#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

#include "chaos.h"

static struct addrinfo *peer;
static int sock = -1;
static int pid;
static struct chaos_udp_packet outgoing;
static struct chaos_udp_packet incoming;
static int lwindow, rwindow;
static void (*receive_packet)(void);
static void (*send_packet)(int opcode, const void *data, size_t len);

typedef void handler_t(const unsigned char *data, int len);

struct handler {
  int opcode;
  handler_t *fn;
};

struct {
  int opcode;
  const char *name;
} chop_name[] = {
  { CHOP_LSN, "LSN" },
  { CHOP_RFC, "RFC" },
  { CHOP_OPN, "OPN" },
  { CHOP_ANS, "ANS" },
  { CHOP_LOS, "LOS" },
  { CHOP_CLS, "CLS" },
  { CHOP_SNS, "SNS" },
  { CHOP_STS, "STS" },
  { CHOP_DAT, "DAT" }
};

#define MAX_OPCODES ((int)(sizeof chop_name / sizeof chop_name[0]))

static void print_udp_packet(FILE *f, struct chaos_udp_packet *data)
{
  int i;

  for (i = 0; i < MAX_OPCODES; i++) {
    if (chop_name[i].opcode == data->opcode) {
      fprintf(f, "%s", chop_name[i].name);
      goto next;
    }
  }
  fprintf(f, "[%03o]", data->opcode);

 next:
  if (data->len > 0) {
    if (data->opcode < 0200) {
      char string[MAX_PACKET];
      strncpy(string, data->data, sizeof string);
      fprintf(f, ": %s\n", string);
    } else {
      fprintf(f, " L%03o", data->len);
    }
  }

  fprintf(f, " #%06o A%06o %06o/%06o->%06o/%06o\n",
          data->pno, data->ano,
          data->laddr, data->lidx,
          data->raddr, data->ridx);
}

static void send_ncp_packet(int opcode, const void *data, size_t len)
{
  if (chaos_packet_send(sock, opcode, data, len) < 0)
    printf("Send error: %s\n", strerror(errno));
}

static void receive_ncp_packet(void)
{
  unsigned char buf[MAX_PACKET];
  int i, opcode;
  ssize_t n = chaos_packet_recv(sock, &opcode, buf);
  if (n == -1)
    printf("Receive error: %s\n", strerror(errno));
  else if (n == 0) {
    printf("Connection closed\n");
    exit(0);
  } else {
    for (i = 0; i < MAX_OPCODES; i++) {
      if (chop_name[i].opcode == opcode) {
        char string[MAX_PACKET];
        strncpy(string, (const char *)buf, n);
        printf("%s: %s\n", chop_name[i].name, string);
        return;
      }
    }
  }
}

static void send_udp_packet(int opcode, const void *data, size_t len)
{
  outgoing.opcode = opcode;
  outgoing.len = len;
  memcpy(outgoing.data, data, len);
  outgoing.pno++;
  if (chaos_udp_send(sock, peer, &outgoing) < 0)
    printf("Send error: %s\n", strerror(errno));
  else
    print_udp_packet(stdout, &outgoing);
}

static void receive_udp_packet(void)
{
  int i;
  i = chaos_udp_recv(sock, &incoming);
  if (i < 0)
    printf("Receive error: %s\n", strerror(errno));
  else if (i == 0) {
    printf("Connection closed\n");
    exit(0);
  } else {
    print_udp_packet(stdout, &incoming);
    printf ("%06o %06o %06o\n", incoming.pno, outgoing.ano,
            (unsigned)incoming.pno - (unsigned)outgoing.ano);
    if ((unsigned)incoming.pno - (unsigned)outgoing.ano < 0100000)
      outgoing.ano = incoming.pno;
    if (incoming.opcode == CHOP_RFC) {
    }
    else if (incoming.opcode == CHOP_OPN) {
      outgoing.raddr = incoming.laddr;
      outgoing.ridx = incoming.lidx;
      rwindow = ((int)incoming.data[2]) << 8;
      rwindow |= incoming.data[3];
    }
    else if (incoming.opcode == CHOP_STS) {
      rwindow = ((int)incoming.data[2]) << 8;
      rwindow |= incoming.data[3];
    }
  }
}

static void read_command(void)
{
  char buf[100];
  char *cmd;
  size_t len;

  cmd = fgets(buf, sizeof buf, stdin);
  len = strlen(cmd);
  if (len > 0 && cmd[len - 1] == '\n')
    cmd[--len] = 0;

  if (strcmp(cmd, "quit") == 0) {
    kill(pid, SIGTERM);
    exit(0);
  }
  else if (strncmp(cmd, "lsn ", 4) == 0) {
    send_packet(CHOP_LSN, cmd + 4, len - 4);
  }
  else if (strncmp(cmd, "rfc ", 4) == 0) {
    memset(&outgoing, 0, sizeof outgoing);
    outgoing.laddr = 0177001;
    outgoing.raddr = 0177002;
    outgoing.lidx = 1;
    outgoing.pno = 0;
    send_packet(CHOP_RFC, cmd + 4, len - 4);
  }
  else if (strcmp(cmd, "opn") == 0) {
    if (len >= 4)
      send_packet(CHOP_OPN, cmd + 4, len - 4);
    else
      send_packet(CHOP_ANS, NULL, 0);
  }
  else if (strcmp(cmd, "ans") == 0) {
    if (len >= 4)
      send_packet(CHOP_ANS, cmd + 4, len - 4);
    else
      send_packet(CHOP_ANS, NULL, 0);
  }
  else if (strcmp(cmd, "dat") == 0) {
  }
  else if (strcmp(cmd, "sts") == 0) {
    unsigned char data[4];
    lwindow = 3;
    data[0] = incoming.pno >> 8;
    data[1] = incoming.pno & 0xFF;
    data[2] = lwindow >> 8;
    data[3] = lwindow & 0xFF;
    send_packet(CHOP_STS, data, sizeof data);
  }
  else if (strcmp(cmd, "sns") == 0) {
    send_packet(CHOP_SNS, NULL, 0);
  }
  else if (strcmp(cmd, "los") == 0) {
    if (len >= 4)
      send_packet(CHOP_LOS, cmd + 4, len - 4);
    else
      send_packet(CHOP_LOS, NULL, 0);
  }
  else if (strcmp(cmd, "cls") == 0) {
    if (len >= 4)
      send_packet(CHOP_CLS, cmd + 4, len - 4);
    else
      send_packet(CHOP_CLS, NULL, 0);
  }
  else if (strcmp(cmd, "") == 0) {
    ;
  }
  else {
    printf("Unknown command: %s\n", cmd);
  }
}

static void process(void)
{
  struct pollfd pfd[2];

  printf(">");
  fflush(stdout);

  pfd[0].fd = fileno(stdin);
  pfd[0].events = POLLIN;
  pfd[1].fd = sock;
  pfd[1].events = POLLIN;

  if (poll(pfd, 2, 10*1000) < 0) {
    printf("\bPoll error: %s\n", strerror(errno));
    return;
  }

  if (pfd[0].revents)
    read_command();
  else if (pfd[1].revents) {
    printf("\b");
    receive_packet();
  }
}

static struct addrinfo *lookup(int type, int flags,
                               const char *host, const char *port)
{
  struct addrinfo hints;
  struct addrinfo *addr;
  struct addrinfo *rp;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = type;
  hints.ai_flags = flags;

  if (getaddrinfo(host, port, &hints, &addr) != 0)
    return NULL;

  for (rp = addr; rp != NULL; rp = rp->ai_next) {
    return rp;
  }
  return NULL;
}

static void usage(char *s)
{
  fprintf(stderr, "Usage: %s [-n] [-u peer]\n", s);
  fprintf(stderr, "  -n  Connect through NCP.\n");
  fprintf(stderr, "  -u  Connect through UDP.\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  char *pname;
  int c;

  pname = argv[0];

  while ((c = getopt(argc, argv, "nu:")) != -1) {
    switch (c) {
    case 'n':
      sock = chaos_packets();
      receive_packet = receive_ncp_packet;
      send_packet = send_ncp_packet;
      break;
    case 'u':
      char lport[100], rport[100];
      snprintf(lport, sizeof lport, "%d", 44041);
      snprintf(rport, sizeof rport, "%d", 44042);
      peer = lookup(SOCK_DGRAM, 0, "localhost", rport);
      sock = chaos_udp(lport);
      if (sock == -1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
      }
      receive_packet = receive_udp_packet;
      send_packet = send_udp_packet;
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

  if (sock == -1) {
    fprintf(stderr, "Must specify one of -n or -u.\n");
    exit(1);
  }

  for (;;)
    process();
}
