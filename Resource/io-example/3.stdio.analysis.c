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
  printf("Standard I/O stream analysis\n"
         "  file:            %s\n"
         "  repetitions:     %ld\n"
         "  I/O size:        %ld\n"
         "\n",
         argv[1], reps, size);

  //
  // read <size> bytes from <file> <reps>-times
  // and print the position of the stream and the underlying file
  //
  long int c = 0;
  int fd = fileno(f);
  while (c < reps) {
    printf("  STREAM offset: %10ld, file position: %10ld\n", ftell(f), lseek(fd, 0, SEEK_CUR));

    size_t byte_read = fread(buf, size, 1, f) * size;
    c++;
    if (byte_read < size) {
      fprintf(stderr, "Cannot read %ld bytes from stream (got %ld bytes).\n", size, byte_read);
      break;
    }
  }

  //
  // close file
  //
  fclose(f);


  //
  // That's all, folks!
  //
  return EXIT_SUCCESS;
}
