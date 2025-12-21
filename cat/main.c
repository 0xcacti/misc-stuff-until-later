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
  fprintf(stderr, "Usage: mcat [file]\n");
}

static void error_msg(const char *filename) {
  fprintf(stderr, "mcat: %s: %s\n", filename, strerror(errno));
}

static void write_all(int fd, const void *buf, size_t len) {
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
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
      error_msg(filename);
      continue;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
      error_msg(filename);
      close(fd);
      exit_code = 1;
      continue;
    }
    size_t sz = st.st_size;
    if (sz == 0) {
      close(fd);
      exit_code = 1;
      continue;
    }

    void *p = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) {
      error_msg(filename);
      close(fd);
      exit_code = 1;
      continue;
    }

    if (write(1, p, sz) < 0) {
      error_msg(filename);
      close(fd);
      exit_code = 1;
      continue;
    }

    if (munmap(p, sz) < 0) {
      error_msg(filename);
      close(fd);
      exit_code = 1;
      continue;
    }
    close(fd);
  }
  return exit_code;
}
