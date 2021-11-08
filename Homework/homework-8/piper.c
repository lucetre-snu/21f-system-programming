//--------------------------------------------------------------------------------------------------
// System Programming                     Homework #8                                    Fall 2021
//
/// @file
/// @brief Summarize size of files in a directory tree
/// @author Sangjun Son
/// @studid 2016-19516

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

void child(int pfd[2], char *dir)
{
  // close unused read end and redirect
  // standard out to write end of pipe
  close(pfd[READ]);
  dup2(pfd[WRITE], STDOUT_FILENO);
  // arguments for execv call
  char *argv[] = {
    "/usr/bin/find",
    dir,
    "-type",
    "f",
    "-printf",
    "%s %f\n",
    NULL,
  };
  // exec does not return on success
  execv(argv[0], argv);
  // exec failed
  exit(EXIT_FAILURE);
}

void parent(int pfd[2])
{
  // close unused write end
  close(pfd[WRITE]);
  // read from pipe
  char filename[1000], large_filename[1000];
  int size, large_size = 0, fcnt = 0, fsize = 0;
  FILE *fp1 = fdopen(pfd[READ], "r");
  while (fscanf(fp1, "%d %s", &size, filename) != -1) {
    if (size > large_size) {
      large_size = size;
      strcpy(large_filename, filename);
    }
    fsize += size;
    fcnt++;
  }
  
  FILE *fp2 = fdopen(STDOUT_FILENO, "w");
  fprintf(fp2, "Found %d files with a total size of %d bytes.\n", fcnt, fsize);
  fprintf(fp2, "The largest file is '%s' with a size of %d bytes.\n", large_filename, large_size);
  
  fclose(fp1);
  fclose(fp2);
  exit(EXIT_SUCCESS);
}

/// @brief program entry point
int main(int argc, char *argv[])
{
  char *dir = (argc > 1 ? argv[1] : ".");
  
  int pipefd[2];
  pid_t pid;
  if (pipe(pipefd) < 0) {
    printf("Cannot create pipe.\n");
    exit(EXIT_FAILURE);
  }
  pid = fork();
  if (pid > 0) parent(pipefd);
  else if (pid == 0) child(pipefd, dir);
  else printf("Cannot fork.\n");
  
  //
  // That's all, folks!
  //
  return EXIT_SUCCESS;
}
