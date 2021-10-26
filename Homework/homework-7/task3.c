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

/// @brief program entry point
int main(int argc, char *argv[], char *envp[])
{
  printf("%s", argv[1]);
  char *path;
  asprintf(&path, "/bin/%s", argv[1]);
  if (execve(path, argv+1, envp) < 0) {
    perror("Cannot execute program");
    exit(EXIT_FAILURE);
  }
  
  return EXIT_SUCCESS;
}
