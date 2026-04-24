#include "stubs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void _start(void) {
  // basic malloc and free
  int *arr = malloc(10 * sizeof(int));
  for (int i = 0; i < 10; i++)
    arr[i] = i * i;
  printf("Squares:\n");
  for (int i = 0; i < 10; i++)
    printf("  %d^2 = %d\n", i, arr[i]);
  free(arr);

  // dynamic string
  char *s = malloc(64);
  strcpy(s, "Hello from heap!");
  printf("%s\n", s);
  free(s);

  // linked list
  typedef struct Node {
    int val;
    struct Node *next;
  } Node;

  Node *head = NULL;
  for (int i = 5; i >= 1; i--) {
    Node *n = malloc(sizeof(Node));
    n->val = i;
    n->next = head;
    head = n;
  }
  printf("Linked list: ");
  Node *cur = head;
  while (cur) {
    printf("%d ", cur->val);
    Node *tmp = cur;
    cur = cur->next;
    free(tmp);
  }
  printf("\n");

  // realloc
  int *buf = malloc(4 * sizeof(int));
  for (int i = 0; i < 4; i++)
    buf[i] = i;
  buf = realloc(buf, 8 * sizeof(int));
  for (int i = 4; i < 8; i++)
    buf[i] = i;
  printf("Realloc buffer: ");
  for (int i = 0; i < 8; i++)
    printf("%d ", buf[i]);
  printf("\n");
  free(buf);

  exit(0);
}