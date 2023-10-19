#ifndef CHAOS_H
#define CHAOS_H

#define MAX_PACKET 492

int chaos_stream(void);
int chaos_rfc(int fd, const char *host, const char *contact);

#endif /* CHAOS_H */
