//--------------------------------------------------------------------------------------------------
// System Programming                     Homework #2                                    Fall 2021
//
/// @file
/// @brief main file
/// @author Sangjun Son
/// @studid 2016-19516
#include <stdio.h>
#include <stdlib.h>
#include "mathlib.h"

/// @brief program entry point
int main(int argc, char *argv[])
{
  // first print all command line arguments
  printf("Command line arguments (%d total):\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("%d: '%s'\n", i, argv[i]);
  }

  // if there is at least one command line argument,
  // add them all up and print the sum.
  int sum = 0;
  for (int i = 1; i < argc; i++) {
    sum = add(sum, atoi(argv[i]));
  }
  printf("\nSum of arguments: %d.\n", sum);
  return EXIT_SUCCESS;
}
