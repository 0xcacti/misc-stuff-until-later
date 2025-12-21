#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum { COUNT_OK, COUNT_INVALID, COUNT_RANGE } count_e;

static void usage() {
  dprintf(STDERR_FILENO, "Usage: head [-n lines | -c bytes] [file ...]\n");
}

static void error_errno(const char *progname, const char *filename) {
  dprintf(STDERR_FILENO, "%s: %s: %s\n", progname, filename, strerror(errno));
}

static void error_msg(const char *progname, const char *msg) {
  dprintf(STDERR_FILENO, "%s: %s\n", progname, msg);
}

static count_e parse_count(const char *s, int *out) {
  errno = 0;
  char *end = NULL;
  long val = strtol(s, &end, 10);
  if (end == s || *end != '\0') return COUNT_INVALID;
  if (errno == ERANGE || val > INT_MAX) return COUNT_RANGE;
  if (val <= 0) return COUNT_INVALID;

  *out = (int)val;
  return COUNT_OK;
}

int main(int argc, char *argv[]) {
  int ch;

  int lc = 0;
  int cc = 0;

  while ((ch = getopt(argc, argv, "n:c:")) != -1) {
    switch (ch) {
    case 'n': {
      count_e res = parse_count(optarg, &lc);
      if (res != COUNT_OK) {
        dprintf(STDERR_FILENO, "%s: illegal line count -- %s\n", argv[0], optarg);
        return 1;
      }
      break;
    }
    case 'c': {
      count_e res = parse_count(optarg, &cc);
      if (res != COUNT_OK) {
        dprintf(STDERR_FILENO, "%s: illegal byte count -- %s\n", argv[0], optarg);
        return 1;
      }
    }
    default:
      usage();
    }

    if (lc > 0 && cc > 0) {
      error_msg(argv[0], "can't combine line and byte counts");
      return 1;
    }
  }
}
