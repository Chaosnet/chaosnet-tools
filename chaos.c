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
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include "chaos.h"

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
    return -1;

  return sock;
}

int chaos_stream(void)
{
  return connect_to_named_socket(SOCK_STREAM, "chaos_stream");
}

int chaos_stream_rfc_data(int fd, const char *host, const char *contact,
                          void *data, size_t len)
{
  char buffer[500];
  ssize_t n;

  if (dprintf(fd, "RFC %s %s", host, contact) < 0)
    return -1;

  if (len > 0) {
    char space = ' ';
    if (write(fd, &space, 1) != 1)
      return -1;
    if (write(fd, data, len) != (ssize_t)len)
      return -1;
  }
  if (dprintf(fd, "\r\n") < 0)
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

int chaos_stream_rfc(int fd, const char *host, const char *contact)
{
  return chaos_stream_rfc_data(fd, host, contact, NULL, 0);
}

int chaos_packets(void)
{
  return connect_to_named_socket(SOCK_STREAM, "chaos_packet");
}

ssize_t chaos_packet_recv(int fd, int *opcode, void *buffer)
{
  unsigned char *data;
  ssize_t n, length;
  int len;

  data = buffer;
  for (len = 4; len > 0; len -= n) {
    n = recv(fd, data, len, 0);
    if (n <= 0)
      return n;
    data += n;
  }

  data = buffer;
  *opcode = data[0];
  length = data[2] | ((int)data[3] << 8);

  for (len = 0; len < length; len += n) {
    n = recv(fd, data, length - len, 0);
    if (n == 0)
      return len;
    if (n < 0)
      return n;
    data += n;
  }

  return len;
}

ssize_t chaos_packet_send(int fd, int opcode, const void *data, size_t len)
{
  const unsigned char *p;
  unsigned char buf[4];
  ssize_t n;
  size_t m;

  buf[0] = opcode;
  buf[1] = 0;
  buf[2] = len & 0xFF;
  buf[3] = (len >> 8) & 0xFF;

  p = buf;
  for (m = sizeof buf; m > 0; m -= n) {
    n = send(fd, p, m, 0);
    if (n <= 0)
      return n;
    p += n;
  }

  p = data;
  for (m = len; m > 0; m -= n) {
    n = send(fd, p, m, 0);
    if (n < 0)
      return n;
    if (n == 0)
      return len - m;
    p += n;
  }

  return len;
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

int chaos_udp(const char *port)
{
  struct addrinfo *addr;
  int reuse = 1;
  int fd;

  addr = lookup(SOCK_DGRAM, AI_PASSIVE, NULL, port);
  if (addr == NULL)
    return -1;
  fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (fd < 0)
    return -1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) < 0)
    return -1;
  if (bind(fd, addr->ai_addr, addr->ai_addrlen) < 0)
    return -1;
  {
    char h[100], s[100];
    getnameinfo(addr->ai_addr, addr->ai_addrlen, h, sizeof h, s, sizeof s, 0);
    fprintf(stderr, "Listen to %s %s\n", h, s);
  }

  freeaddrinfo(addr);

  return fd;
}

static int word(unsigned char *data)
{
  int x = data[0];
  x <<= 8;
  return x | data[1];
}

int chaos_udp_recv(int fd, struct chaos_udp_packet *packet)
{
  unsigned char data[MAX_PACKET + 26];
  ssize_t n = recv(fd, data, sizeof data, 0);
  int i;
  if (n <= 0)
    return n;
  packet->opcode = data[4];
  packet->len = word(data + 6);
  packet->raddr = word(data + 8);
  packet->ridx = word(data + 10);
  packet->laddr = word(data + 12);
  packet->lidx = word(data + 14);
  packet->pno = word(data + 16);
  packet->ano = word(data + 18);
  memcpy(packet->data, data + 20, packet->len);
  for (i = 0; i < packet->len; i+=2) {
    packet->data[i+1] = data[i + 20];
    packet->data[i+0] = data[i + 21];
  }
  return n;
}

int sum(unsigned char *data, int n)
{
  int sum = 0;

  while (n > 1) {
    sum += (((int)data[0]) << 8) | data[1];
    data += 2;
    n -= 2;
  }

  /* Add left-over byte, if any. */
  if (n > 0)
    sum += *data;

  /* Fold 32-bit sum to 16 bits. */
  while (sum & ~0xFFFF)
    sum = (sum & 0xFFFF) + (sum >> 16);

  return (~sum) & 0xFFFF;
}

int chaos_udp_send(int fd, const struct addrinfo *addr,
                   struct chaos_udp_packet *packet)
{
  unsigned char data[MAX_PACKET + 26];
  int checksum, len = 0, i;
  data[len++] = 1;
  data[len++] = 1;
  data[len++] = 0;
  data[len++] = 0;
  data[len++] = packet->opcode;
  data[len++] = 0;
  data[len++] = (packet->len >> 8) & 0x0F;
  data[len++] = packet->len & 0xFF;
  data[len++] = packet->raddr >> 8;
  data[len++] = packet->raddr & 0xFF;
  data[len++] = packet->ridx >> 8;
  data[len++] = packet->ridx & 0xFF;
  data[len++] = packet->laddr >> 8;
  data[len++] = packet->laddr & 0xFF;
  data[len++] = packet->lidx >> 8;
  data[len++] = packet->lidx & 0xFF;
  data[len++] = packet->pno >> 8;
  data[len++] = packet->pno & 0xFF;
  data[len++] = packet->ano >> 8;
  data[len++] = packet->ano & 0xFF;
  for (i = 0; i < packet->len; i+=2) {
    data[len++] = packet->data[i+1];
    data[len++] = packet->data[i+0];
  }
  data[len++] = packet->raddr >> 8;
  data[len++] = packet->raddr & 0xFF;
  data[len++] = packet->laddr >> 8;
  data[len++] = packet->laddr & 0xFF;
  checksum = sum(data + 4, len - 4);
  data[len++] = checksum >> 8;
  data[len++] = checksum & 0xFF;
  {
    char h[100], s[100];
    getnameinfo(addr->ai_addr, addr->ai_addrlen, h, sizeof h, s, sizeof s, 0);
    fprintf(stderr, "Send to %s %s\n", h, s);
  }
  return sendto(fd, data, len, 0, addr->ai_addr, addr->ai_addrlen);
}
