#include <stdio.h>
#include <stdlib.h>
#include "fib.h"

int main(int argc, char *argv[]) {
  int n = atoi(argv[1]);
  printf("fibonacci(%d) = %ld\n", n, fibonacci(n));
  return EXIT_SUCCESS;
}

