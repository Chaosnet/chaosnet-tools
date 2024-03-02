#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int
server_socket (const char *host, int port, int backlog)
{
  struct sockaddr_in address;
  int i, fd;

  memset (&address, '\0', sizeof address);
#if defined(__FreeBSD__) || defined(__OpenBSD__)
  address.sin_len = sizeof address;
#endif
  address.sin_family = PF_INET;
  address.sin_port = htons ((unsigned short)port);
  address.sin_addr.s_addr = host ? inet_addr (host) : INADDR_ANY;

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
  
  i = 1;
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (void *)&i, sizeof i);

  if (bind (fd, (struct sockaddr *)&address, sizeof address) == -1)
    {
      close (fd);
      return -1;
    }

  if (listen (fd, (unsigned)backlog) == -1)
    {
      close (fd);
      return -1;
    } 

  return fd;
}
