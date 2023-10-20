#ifndef CHAOS_H
#define CHAOS_H

#include <sys/types.h>

enum { CHOP_RFC=1, CHOP_OPN, CHOP_CLS, CHOP_FWD, CHOP_ANS, CHOP_SNS, CHOP_STS,
       CHOP_RUT, CHOP_LOS, CHOP_LSN, CHOP_MNT, CHOP_EOF, CHOP_UNC, CHOP_BRD };

#define CHOP_ACK 0177 // Note: extension for the NCP Packet socket
#define CHOP_DAT 0200
#define CHOP_DWD 0300

#define MAX_PACKET 492

int chaos_stream(void);
int chaos_packets(void);
int chaos_rfc(int fd, const char *host, const char *contact);
ssize_t chaos_recv(int fd, int *opcode, void *buffer);
ssize_t chaos_send(int fd, int opcode, const void *data, size_t len);

#endif /* CHAOS_H */
