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
#include <signal.h>
#include <sys/wait.h>
volatile int global_counter;

void parent() {
  printf("[%d] Hello from parent.\n", getpid());
  printf("[%d]   Waiting for child to terminate...\n", getpid());
    
  int child_status;
  pid_t wpid = wait(&child_status);
  if (WIFEXITED(child_status))
    printf("[%d] Child %d has terminated normally. It has received %d SIGUSR1 signals.\n", getpid(), wpid, WEXITSTATUS(child_status));
  else
    printf("[%d] Child %d has terminated abnormally\n", getpid(), wpid);
}

void hdl_sigusr1() {
  global_counter++;
  printf("[%d] Child received SIGUSR1! Count = %d.\n", getpid(), global_counter);
}

void hdl_sigusr2() {
  exit(global_counter);
}

void child() {
  printf("[%d] Hello from child.\n", getpid());
  
  if (signal(SIGUSR1, hdl_sigusr1) == SIG_ERR)
    perror("Cannot install SIGUSR1 handler.");
  printf("[%d]   SIGUSR1 handler installed.\n", getpid());

  if (signal(SIGUSR2, hdl_sigusr2) == SIG_ERR)
    perror("Cannot install SIGUSR2 handler.");
  printf("[%d]   SIGUSR2 handler installed.\n", getpid());
  
  printf("[%d]   Waiting for signals...\n", getpid());
  while (1);
}

/// @brief program entry point
int main(int argc, char *argv[])
{
  if (fork()) parent();
  else child();

  return EXIT_SUCCESS;
}