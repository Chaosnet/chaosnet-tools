/* Copyright © 2020 Björn Victor (bjorn@victor.se) */
/* Copyright © 2023 Lars Brinkoff <lars@nocrew.org> */

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
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "chaos.h"

#define CHUDP_HEADER    4
#define CHUDP_VERSION   1
#define CHUDP_PKT       1
#define CHAOS_HEADER   10

static const char *chaos_socket_directory = "/tmp";

static int connect_to_named_socket(int type, char *path)
{
  int sock, slen;
  struct sockaddr_un server;
  
  if ((sock = socket(AF_UNIX, type, 0)) < 0)
    return -1;
  
  server.sun_family = AF_UNIX;
  sprintf(server.sun_path, "%s/%s", chaos_socket_directory, path);
  slen = strlen(server.sun_path)+ 1 + sizeof(server.sun_family);

  if (connect(sock, (struct sockaddr *)&server, slen) < 0)
    return 0;

  return sock;
}

int chaos_stream(void)
{
  return connect_to_named_socket(SOCK_STREAM, "chaos_stream");
}

int chaos_rfc(int fd, const char *host, const char *contact)
{
  char buffer[500];
  ssize_t n;

  if (dprintf(fd, "RFC %s %s\r\n", host, contact) < 0)
    return -1;

  n = read(fd, buffer, sizeof buffer);
  if (n < 0)
    return n;
  if (n == 0) {
    errno = ECONNABORTED;
    return -1;
  }

  if (strncmp(buffer, "OPN ", 4) != 0) {
    errno = ECONNABORTED;
    return -1;
  }

  return 0;
}

int chaos_udp(int lport, const char *peer, int rport)
{
  struct sockaddr_in sa;
  struct addrinfo hints;
  struct addrinfo *addr;
  struct addrinfo *rp;
  char str[10];
  int reuse = 1;
  int fd;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0)
    return -1;

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) < 0)
    goto error;

  sa.sin_family = AF_INET;
  sa.sin_port = htons(lport);
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(fd, (struct sockaddr *)&sa, sizeof sa) < 0)
    goto error;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  snprintf(str, sizeof str, "%d", rport);
  if (getaddrinfo(peer, str, &hints, &addr) != 0)
    goto error;

  for (rp = addr; rp != NULL; rp = rp->ai_next) {
    if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      freeaddrinfo(addr);
      return fd;
    }
  }

error:
  close(fd);
  return -1;
}

ssize_t chaos_udp_send(int fd, int opcode, const void *data, size_t len)
{
  unsigned char header[CHUDP_HEADER + CHAOS_HEADER];
  ssize_t n;

  memset(header, 0, sizeof header);
  header[0] = CHUDP_VERSION;
  header[1] = CHUDP_PKT;
  header[CHUDP_HEADER + 0] = opcode;
  header[CHUDP_HEADER + 2] = len & 0xFF;
  header[CHUDP_HEADER + 3] = (len >> 8) & 0xF;
  n = send(fd, header, sizeof header, 0);

  n = send(fd, data, len, 0);
  return n;
}

ssize_t chaos_udp_recv(int fd, int *opcode, void *data)
{
  unsigned char packet[CHUDP_HEADER + CHAOS_HEADER + MAX_PACKET];
  ssize_t n;

  n = recv(fd, packet, sizeof packet, 0);
  *opcode = packet[CHUDP_HEADER + 0];
  n -= CHUDP_HEADER + CHAOS_HEADER;
  memcpy(data, packet + CHUDP_HEADER + CHAOS_HEADER, n);
  return n;
}
