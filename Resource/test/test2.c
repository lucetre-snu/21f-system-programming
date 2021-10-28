#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>

/*
int main() {
  if (fork() == 0) {
    printf("1\n"); sleep(1); printf("2\n");
  } else {
    printf("3\n"); wait(NULL); printf("4\n");
  }
  return 0;
}*/

/*
int main() {
  if (fork() == 0) {
    for (int i = 0; i < 2; i++) {
      printf("%d\n", getppid()); sleep(2);
    }
  }
  sleep(1);
  return 0;
}*/

#include <string.h>

int main() {
  if (opendir("123") == NULL) {
    printf("%s", strerror(errno));
  }
  return 0;
}
