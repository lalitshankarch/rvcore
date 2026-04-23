#include "stubs.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void write_str(const char *s) { write(1, s, strlen(s)); }

void write_int(int n) {
  if (n < 0) {
    write(1, "-", 1);
    n = -n;
  }
  char buf[12];
  int i = 11;
  buf[i] = '\0';
  if (n == 0) {
    write(1, "0", 1);
    return;
  }
  while (n > 0) {
    buf[--i] = '0' + (n % 10);
    n /= 10;
  }
  write(1, buf + i, 11 - i);
}

int fibonacci(int n) {
  if (n <= 1)
    return n;
  return fibonacci(n - 1) + fibonacci(n - 2);
}

void bubble_sort(int *arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        int tmp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = tmp;
      }
    }
  }
}

void _start(void) {
  write_str("Fibonacci:\n");
  for (int i = 0; i < 10; i++) {
    write_str("  fib(");
    write_int(i);
    write_str(") = ");
    write_int(fibonacci(i));
    write_str("\n");
  }

  write_str("Bubble sort:\n");
  int arr[] = {64, 34, 25, 12, 22, 11, 90};
  int n = 7;
  bubble_sort(arr, n);
  write_str("  sorted: ");
  for (int i = 0; i < n; i++) {
    write_int(arr[i]);
    if (i < n - 1)
      write_str(", ");
  }
  write_str("\n");

  exit(0);
}