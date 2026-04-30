#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int _fstat(int file, struct stat *st) {
  st->st_mode = S_IFREG;
  return 0;
}

int _open(const char *path, int flags) {
  int fd;
  __asm__ __volatile__("mv      a0, %[src1]\n\t"
                       "mv      a1, %[src2]\n\t"
                       "li      a7, 1\n\t"
                       "ecall\n\t"
                       "mv      %[dst], a0\n\t"
                       : [dst] "=r"(fd)
                       : [src1] "r"(path), [src2] "r"(flags)
                       : "a0", "a7");
  return fd;
}

ssize_t _read(int fd, void *buf, size_t count) {
  ssize_t nbytes;
  __asm__ __volatile__("mv      a0, %[src1]\n\t"
                       "mv      a1, %[src2]\n\t"
                       "mv      a2, %[src3]\n\t"
                       "li      a7, 2\n\t"
                       "ecall\n\t"
                       "mv      %[dst], a0\n\t"
                       : [dst] "=r"(nbytes)
                       : [src1] "r"(fd), [src2] "r"(buf), [src3] "r"(count)
                       : "a0", "a1", "a2", "a7");
  return nbytes;
}

off_t _lseek(int fd, off_t offset, int whence) {
  off_t new_offset;
  __asm__ __volatile__("mv      a0, %[src1]\n\t"
                       "mv      a1, %[src2]\n\t"
                       "mv      a2, %[src3]\n\t"
                       "li      a7, 4\n\t"
                       "ecall\n\t"
                       "mv      %[dst], a0\n\t"
                       : [dst] "=r"(new_offset)
                       : [src1] "r"(fd), [src2] "r"(offset), [src3] "r"(whence)
                       : "a0", "a1", "a2", "a7");
}

int _close(int fd) {
  if (fd == 0 || fd == 1 || fd == 2) {
    return 0;
  }
  int ret;
  __asm__ __volatile__("mv      a0, %[src]\n\t"
                       "li      a7, 5\n\t"
                       "ecall\n\t"
                       "mv      %[dst], a0\n\t"
                       : [dst] "=r"(ret)
                       : [src] "r"(fd)
                       : "a0", "a7");
  return ret;
}

pid_t _getpid() { return 0; }

int _isatty(int fd) { return 0; }

int _kill(pid_t pid, int sig) { return -1; }

void _exit(int status) {
  __asm__ __volatile__("li      a7, 10\n\t"
                       "ecall\n\t");
}

ssize_t _write(int fildes, const void *buf, ssize_t nbyte) {
  ssize_t nbytes;
  __asm__ __volatile__("mv      a0, %[src1]\n\t"
                       "mv      a1, %[src2]\n\t"
                       "mv      a2, %[src3]\n\t"
                       "li      a7, 3\n\t"
                       "ecall\n\t"
                       "mv      %[dst], a0\n\t"
                       : [dst] "=r"(nbytes)
                       : [src1] "r"(fildes), [src2] "r"(buf), [src3] "r"(nbyte)
                       : "a0", "a1", "a2", "a7");
  return nbytes;
}

void *_sbrk(intptr_t increment) {
  void *ptr;
  __asm__ __volatile__("mv      a0, %[src]\n\t"
                       "li      a7, 0\n\t"
                       "ecall\n\t"
                       "mv      %[dst], a0\n\t"
                       : [dst] "=r"(ptr)
                       : [src] "r"(increment)
                       : "a0", "a7");
  return ptr;
}