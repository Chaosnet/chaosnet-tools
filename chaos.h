#ifndef CHAOS_H
#define CHAOS_H

#include <sys/types.h>

#define MAX_PACKET 492

int chaos_stream(void);
int chaos_stream_rfc(int fd, const char *host, const char *contact);
int chaos_stream_rfc_data(int, const char *, const char *, void *, size_t);

#endif /* CHAOS_H */
