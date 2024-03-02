#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

static int fd;

static int client_socket (const char *host, int port)
{
  struct sockaddr_in address;
  int fd;

  memset (&address, '\0', sizeof address);
#if defined(__FreeBSD__) || defined(__OpenBSD__)
  address.sin_len = sizeof address;
#endif
  address.sin_family = PF_INET;
  address.sin_port = htons ((unsigned short)port);
  address.sin_addr.s_addr = inet_addr (host);

  if (address.sin_addr.s_addr == INADDR_NONE)
    {
      struct hostent *ent;

      ent = gethostbyname (host);
      if (ent == 0)
	return -1;

      memcpy (&address.sin_addr.s_addr, ent->h_addr, (size_t)ent->h_length);
    }

  fd = socket (PF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    return -1;
  
  if (connect (fd, (struct sockaddr *)&address, sizeof address) == -1)
    {
      close (fd);
      return -1;
    } 

  return fd;
}

void io_init (const char *host, int port)
{
  fd = client_socket (host, port);
  fprintf (stderr, "OK\n");
  if (fd == -1)
    {
      fprintf (stderr, "Error opening connection.\n");
      exit (1);
    }
}

void io_flush (void)
{
  int flag = 1;
  setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof flag);
  flag = 0;
  setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof flag);
}

void io_write (void *data, int n)
{
  ssize_t m = write (fd, data, n);
  if (m != n)
    {
      fprintf (stderr, "Write error: %ld\n", m);
      exit (1);
    }
}

void io_read (void *data, int n)
{
  int m;
  while (n > 0)
    {
      m = read (fd, data, n);
      if (m == -1)
	{
	  fprintf (stderr, "Read error\n");
	  exit (1);
	}
      data += m;
      n -= m;
    }
}


