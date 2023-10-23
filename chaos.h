#ifndef CHAOS_H
#define CHAOS_H

#include <sys/types.h>

#define MAX_PACKET 492

int chaos_stream(void);
int chaos_rfc(int fd, const char *host, const char *contact);

int chaos_udp(int lport, const char *peer, int rport);
ssize_t chaos_udp_send(int fd, int opcode, const void *data, size_t len);
ssize_t chaos_udp_recv(int fd, int *opcode, void *data);

#endif /* CHAOS_H */
