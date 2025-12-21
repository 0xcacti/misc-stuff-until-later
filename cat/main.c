#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void usage() { fprintf(stderr, "Usage: mcat [file]\n"); }

void error_msg(const char *filename) {
  fprintf(stderr, "mcat: %s: %s\n", filename, strerror(errno));
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

  for (int i = optind; i < argc; i++) {
    char *filename = argv[i];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
      if (errno == EACCES) {
        error_msg(filename);
      }
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
      error_msg(filename);
      close(fd);
      continue;
    }
    size_t sz = st.st_size;
    void *p = mmap(0, sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p == MAP_FAILED) {
      error_msg(filename);
      close(fd);
      continue;
    }

    write(0, p, sz);

    // TODO: error
    munmap(p, sz);
  }
}
