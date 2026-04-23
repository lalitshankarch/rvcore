#pragma once

#include <unistd.h>
#include <sys/stat.h>

int _fstat(int file, struct stat *st) {
  st->st_mode = S_IFCHR;
  return 0;
}

ssize_t _read(int fd, void *buf, size_t count) { return 0; }

off_t _lseek(int fd, off_t offset, int whence) { return offset - 1; }

int _close(int fd) { return -1; }

pid_t _getpid() { return 0; }

int _isatty(int fd) { return 1; }

int _kill(pid_t pid, int sig) { return -1; }

void _exit(int status) {
  __asm__ __volatile__("li      a7, 10\n\t"
                       "ecall\n\t");
}

size_t _write(int fildes, const void *buf, size_t nbyte) {
  __asm__ __volatile__("addi    a0, a1, 0\n\t"
                       "li      a7, 4\n\t"
                       "ecall\n\t");
  return nbyte;
}

void *_sbrk(intptr_t increment) { return NULL; }