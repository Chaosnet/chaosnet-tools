/* Copyright © 2024 Björn Victor (bjorn@victor.se) */
/* Copyright © 2023 Lars Brinkoff <lars@nocrew.org> */

/* Program to read records from e.g. an RTAPE tape file, and write it
   back on stdout without the record markers. Useful to get the actual
   contents of such a tape file, e.g. to use with tar, or perhaps with
   LambdaDelta.

   Extend this (or write another program) to do the opposite!
 */

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
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>

#include "chaos.h"
#include "tape-image.h"

#define MAX_RECORD  65536  /* Max size of a tape record we handle. */

static void usage(char *s)
{
  fprintf(stderr, "This program reads records from a file (or standard input)\n"
	  "and writes them without the record markers on standard output.\n");
  fprintf(stderr, "Usage: %s [-a -n N] [input]\n", s);
  fprintf(stderr, "  -a    Read all records (default).\n");
  fprintf(stderr, "  -n N  Read N records.\n");
  fprintf(stderr, "  -v    Verbose output.\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  char *pname;
  int c, read_all = 1, numrecs = 0;
  int verbose = 0;
  FILE *input = stdin;
  char buf[MAX_RECORD];

  pname = argv[0];

  while ((c = getopt(argc, argv, "an:v")) != -1) {
    switch (c) {
    case 'a':
      read_all = 1;
      break;
    case 'n':
      numrecs = atoi(optarg); 
      read_all = 0;
      if (numrecs < 1) {
	fprintf(stderr,"Too few records %s\n", optarg);
	usage(pname);
      }
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      fprintf(stderr, "Unknown option: %c\n", c);
      usage(pname);
    }
  }
  argc -= optind;
  argv += optind;

  if (numrecs + read_all == 0) {
    usage(pname);
  }

  if (argc > 0) {
    input = fopen(argv[0],"r");
    if (input == NULL) {
      perror("fopen");
      exit(1);
    }
  }
  int recnum = 0, lastwasmark = 0;
  while ((read_all == 1) || (numrecs-- > 0)) {
    size_t m = read_record(fileno(input), buf, sizeof(buf));
    if (m & RECORD_ERR) {
      fprintf(stderr,"Error reading record %d!\n", recnum);
      exit(1);
    } else if (m == RECORD_EOM) {
      if (verbose) 
	fprintf(stderr,"EOM: Read %d records.\n", recnum);
      exit(0);
    } else if (m == RECORD_MARK) {
      // ignore
      if (lastwasmark) {
	if (verbose) {
	  fprintf(stderr,"[Second mark: EOF]\n");
	  fprintf(stderr,"Done: read %d records.\n", recnum);
	}
	exit(0);
      }
      if (verbose)
	fprintf(stderr,"[Mark]\n");
      lastwasmark = 1;
    } else {
      lastwasmark = 0;
      if (verbose)
	fprintf(stderr,"[Rec #%d: %zd bytes]\n", recnum, m);
      if (write(fileno(stdout), buf, m) < 0) {
	perror("Error writing output");
	exit(1);
      }
    }
    recnum++;
  }
  if (verbose)
    fprintf(stderr,"Done: read %d records.\n", recnum);
}


