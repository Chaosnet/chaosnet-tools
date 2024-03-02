#include "libword.h"

#define MLDEV_DIR -1
#define MLDEV_LINK -2

#define DIR_MAX 204 /* Maximum number of files in a directory. */

/* This seems to be a good size for file read requests.  Larger sizes
   seem to confuse MLSLV to trigger RDATA replies to be bigger than
   the requested by CALLOC.  */
#define MAX_READ 0400

#define MAX_WRITE 0200

struct mldev_file
{
  char device[7], sname[7];
  char fn1[7], fn2[7];
  int pack;
  int mdate, mtime, rdate;
};

extern void mldev_init (const char *);
extern int mldev_open (const word_t *, int);
extern int mldev_close (void);
extern int mldev_read (word_t *, int);
extern int mldev_write (const word_t *, int);
