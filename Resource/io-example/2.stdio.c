#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

long int parse_number(const char *str)
{
  long int res;
  char *endpos;

  //
  // reset errno and call strtol()
  //
  errno = 0;
  res = strtol(str, &endpos, 0);

  //
  // check for parsing errors
  //
  if (errno != 0) {
    fprintf(stderr, "Cannot parse number '%s': ", str);
    perror(NULL);
    exit(EXIT_FAILURE);
  }

  if ((endpos == str) || (*endpos != '\0')) {
    fprintf(stderr,"Invalid characters in number '%s'.\n", str);
    exit(EXIT_FAILURE);
  }

  //
  // all good, return number
  //
  return res;
}


int main(int argc, char *argv[])
{
  //
  // check that necessary command line arguments were supplied
  //
  if (argc != 4) {
    fprintf(stderr, "Syntax: %s <file> <reps> <size>\n", basename(argv[0]));
    fprintf(stderr, "where\n"
                    "  <file>          file to read from\n"
                    "  <reps>          number of repetitions\n"
                    "  <size>          size of one I/O operation\n"
                    "\n");
    return EXIT_FAILURE;
  }

  //
  // open file
  //
  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    perror("Cannot open file");
    return EXIT_FAILURE;
  }

  //
  // parse <reps> and <size> parameters
  // and get memory for the buffer
  //
  long int reps = parse_number(argv[2]);
  size_t size = parse_number(argv[3]);
  char *buf = malloc(size);

  if (buf == NULL) {
    fprintf(stderr, "Cannot allocated %ld bytes for buffer.\n", size);
    fclose(f);
    return EXIT_FAILURE;
  }


  //
  // say what we are about to do
  //
  printf("Standard I/O test\n"
         "  file:            %s\n"
         "  repetitions:     %ld\n"
         "  I/O size:        %ld\n"
         "\n",
         argv[1], reps, size);

  //
  // start timer
  //
  struct timespec ts, te;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);

  //
  // read <size> bytes from <file> <reps>-times
  //
  long int c = 0;
  while (c < reps) {
    size_t byte_read = fread(buf, size, 1, f);
    c++;
    if (byte_read < size) break;
  }

  //
  // stop timer & compute time difference
  //
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &te);

  te.tv_sec  -= ts.tv_sec;
  te.tv_nsec -= ts.tv_nsec;
  if (te.tv_nsec < 0) {
    te.tv_nsec += 1000000000;
    te.tv_sec --;
  }

  //
  // close file
  //
  fclose(f);


  //
  // print result
  //
  printf("  Completed %ld I/O operations in %ld.%06ld seconds.\n", c, te.tv_sec, te.tv_nsec);


  //
  // That's all, folks!
  //
  return EXIT_SUCCESS;
}
