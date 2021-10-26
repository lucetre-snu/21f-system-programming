#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXPROC 16

// abort process with an optional error message
void ABORT(char *msg)
{
  if (msg) { printf("%s\n", msg); fflush(stdout); }
  abort();
}

int main(int argc, char *argv[])
{
  //
  // convert argument at command line into number
  //
  if (argc != 2) ABORT("Missing argument.");
  int nproc = atoi(argv[1]);

  if (nproc < 1) nproc = 1;
  if (nproc > MAXPROC) nproc = MAXPROC;

  //
  // TODO
  //
  
  pid_t pid[MAXPROC+1];
  for (int i = 1; i <= nproc; i++) { 
    if ((pid[i] = fork()) == 0) {
      char *argv;
      if (asprintf(&argv, "%d", i) == -1) ABORT("Memory error");
      execl("child", "child", argv, (char *)NULL);
    }
  }

  for (int i = 1; i <= nproc; i++) {
    if (pid[i]) {
      int status;
      waitpid(pid[i], &status, 0);
      if (WIFEXITED(status)) {
        printf("Child %d terminated normally with exit code %d.\n", pid[i], WEXITSTATUS(status));
      }
    }
  }


  //
  // that's all, folks!
  //
  return EXIT_SUCCESS;
}
