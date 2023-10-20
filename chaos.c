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
    return 0;

  return sock;
}

int chaos_stream(void)
{
  return connect_to_named_socket(SOCK_STREAM, "chaos_stream");
}

int chaos_packets(void)
{
  return connect_to_named_socket(SOCK_STREAM, "chaos_packet");
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

ssize_t chaos_recv(int fd, int *opcode, void *buffer)
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

ssize_t chaos_send(int fd, int opcode, const void *data, size_t len)
{
  unsigned char buf[4];
  ssize_t n;

  buf[0] = opcode;
  buf[1] = 0;
  buf[2] = len & 0xFF;
  buf[3] = (len >> 8) & 0xFF;
  n = send(fd, buf, sizeof buf, 0);
  if (n <= 0)
    return n;
  if (len > 0) {
    n = send(fd, data, len, 0);
    if (n <= 0)
      return n;
  }

  return len;
}
