#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static void usage() {
  dprintf(STDERR_FILENO, "Usage: mcat [file]\n");
}

static void error_msg(const char *filename) {
  dprintf(STDERR_FILENO, "mcat: %s: %s\n", filename, strerror(errno));
}

static int write_all(int fd, const void *buf, size_t len) {
  size_t off = 0;
  while (off < len) {
    ssize_t n = write(fd, buf + off, len - off);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (n == 0) {
      errno = EIO;
      return -1;
    }
    off += (size_t)n;
  }

  return 0;
}

int cat_file(const char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    error_msg(filename);
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    error_msg(filename);
    close(fd);
    return -1;
  }
  size_t sz = st.st_size;
  if (sz == 0) {
    close(fd);
    return 0;
  }

  void *p = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0);
  if (p == MAP_FAILED) {
    error_msg(filename);
    close(fd);
    return -1;
  }

  if (write_all(STDOUT_FILENO, p, sz) < 0) {
    error_msg(filename);
    close(fd);
    return -1;
  }

  if (munmap(p, sz) < 0) {
    error_msg(filename);
    close(fd);
    return -1;
  }
  if (close(fd) < 0) {
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int ch;

  while ((ch = getopt(argc, argv, "")) != -1) {
    switch (ch) {
    case '?':
    default:
      usage();
      return 1;
    }
  }

  int exit_code = 0;
  for (int i = optind; i < argc; i++) {
    char *filename = argv[i];
    if (cat_file(filename) < 0) {
      exit_code = 1;
    }
  }
  return exit_code;
}
