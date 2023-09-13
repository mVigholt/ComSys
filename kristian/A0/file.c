#include <stdio.h>  // fprintf, stdout, stderr.
#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS.
#include <string.h> // strerror.
#include <errno.h>  // errno.
#include <strings.h> //string type
#define BUFFER_SIZE  26


int print_hello_world(void) {
  return fprintf(stdout, "Hello, world!\n");
}


// Assumes: errnum is a valid error number
int print_error(char *path, int errnum) {
  return fprintf(stdout, "%s: cannot determine (%s)\n",
    path, strerror(errnum));
}


// opens file for reading and returns count of characters read
// see: https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-fread-read-items
size_t read_file(char* file_path) {
  size_t count;
  char buffer[BUFFER_SIZE + 1];
  buffer[BUFFER_SIZE] = '\0';

  FILE *f = fopen(file_path, "r");

  // 2.3 error if filepath or name doesnt exist
  if (!f) {
    print_error(file_path, errno);
    return EXIT_FAILURE;
  } 
  
  count = fread(buffer, sizeof( char ), 100, f);
  fclose(f);

  return count;
}


int main(int argc, char* argv[argc+1]) {

  // 2.2 return stderr if no input path
  if (argc == 1) {
    fprintf(stderr, "Usage: file path");
    return EXIT_FAILURE;
  };

  char* file_path = argv[1];
  size_t num;
  char buffer[BUFFER_SIZE + 1];
  buffer[BUFFER_SIZE] = '\0';

  FILE *f = fopen(file_path, "r");

  // 2.3 error if filepath or name doesnt exist
  if (!f) {
    print_error(file_path, errno);
    return EXIT_SUCCESS;
  }
  
  num = fread(buffer, sizeof( char ), 100, f);
  fclose(f);
  
  if (!num) {
    printf("empty");
  } else {
    printf("%zu", num);
  }

  return EXIT_SUCCESS;
}
