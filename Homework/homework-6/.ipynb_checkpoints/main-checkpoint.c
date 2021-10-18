#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
  int i;
  pid_t pid = getpid();
  
  for (i=0; i<3; i++) {
    printf("Hello %d.\n", i);
    
    pid = fork();
    if (i % 2 == 0) {
      if (pid == 0) break;
    } else {
      if (pid > 0) break;
    }
  }
  if (pid > 0) waitpid(pid, NULL, 0);
  printf("Bye %d.\n", i);
  
  return 0;
}
