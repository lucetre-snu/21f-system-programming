//--------------------------------------------------------------------------------------------------
// System Programming                     Homework #7                                    Fall 2021
//
/// @file
/// @brief template
/// @author Sangjun Son
/// @studid 2016-19516
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void parent() {
  printf("[%d] Hello from parent.\n", getpid());
}

void child() {
  printf("[%d] Hello from child.\n", getpid());
}

/// @brief program entry point
int main(int argc, char *argv[])
{
  if (fork()) parent();
  else child();

  return EXIT_SUCCESS;
}
