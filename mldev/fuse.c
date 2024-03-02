#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "mldev.h"

static int mlfuse_getattr (const char *path, struct stat *stbuf)
{
  struct mldev_file file;
  int n;

  n = mldev_getattr (path, &file);
  if (n < 0)
    return n;

  memset (stbuf, 0, sizeof (struct stat));
  if (file.pack == MLDEV_DIR)
    {
      stbuf->st_mode = S_IFDIR | 0777;
      stbuf->st_nlink = 2;
    }
  else
    {
      stbuf->st_mode = S_IFREG | 0777;
      stbuf->st_nlink = 1;
    }

  return 0;
}

static int mlfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
  struct mldev_file files[DIR_MAX];
  int i, n;
  
  n = mldev_readdir (path, files);
  if (n < 0)
    return n;

  filler (buf, ".", NULL, 0);
  filler (buf, "..", NULL, 0);

  for (i = 0; i < n; i++)
    {
      char name[14];
      if (files[i].pack == MLDEV_DIR)
	sprintf (name, "%s", files[i].fn1);
      else
	sprintf (name, "%s %s", files[i].fn1, files[i].fn2);
      filler (buf, name, NULL, 0);
    }

  return 0;
}

static int mlfuse_truncate(const char *path, off_t size)
{
  if (size != 0)
    return -EIO;
  return 0;
}

static int mlfuse_open(const char *path, struct fuse_file_info *fi)
{
  int n;

  /* In the default case, FUSE looks at st_size to see the size of
     the file.  With MLDEV, this takes some effort.  To avoid that,
     we can set the direct_io flag. */
  fi->direct_io = 1;

  fi->nonseekable = 1; /* Not sure about this. */

  n = mldev_open (path, fi->flags & 3);
  if (n < 0)
    return n;

  return 0;
}

static int mlfuse_close(const char *path, struct fuse_file_info *fi)
{
  return mldev_close (path);
}

static int mlfuse_write(const char *path, const char *buf, size_t size,
		       off_t offset, struct fuse_file_info *fi)
{
  if (size == 0)
    return 0;

  return mldev_write (path, buf, size, offset);
}

static int mlfuse_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
  if (size == 0)
    return 0;

  return mldev_read (path, buf, size, offset);
}

static int mlfuse_readlink (const char *path, char *data, size_t n)
{
  fprintf (stderr, "readlink: %s\n", path);
  strcpy (data, "foo/bar");
  return 7;
}

static struct fuse_operations mldev =
{
  .getattr	= mlfuse_getattr,
  .readdir	= mlfuse_readdir,
  .truncate	= mlfuse_truncate,
  .open		= mlfuse_open,
  .release	= mlfuse_close,
  .write	= mlfuse_write,
  .read		= mlfuse_read,
  .readlink	= mlfuse_readlink,
};

int main (int argc, char **argv)
{
  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s <host> [options] <directory>\n", argv[0]);
      exit (1);
    }

  mldev_init (argv[1]);

  argc--;
  argv++;

  return fuse_main(argc, argv, &mldev, NULL);
}
