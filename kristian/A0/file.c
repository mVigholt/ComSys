#include <stdio.h>  // fprintf, stdout, stderr.
#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS.
#include <string.h> // strerror.
#include <errno.h>  // errno.
#include <strings.h> //string type
#define BUFFER_SIZE  27


// Enums of file types for identifying the files
enum {
  DATA,
  EMPTY,
  ASCII,
  ISO8859,
  UTF8
};

// Strings of file types for printing to stdout
const char* const FILE_TYPE_STRINGS[] = {
  "data",
  "empty",
  "ASCII text",
  "ISO-8859-1 text",
  "UTF-8 text"
};


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

  // 2.2 return stderr if no input path or too many arguments
  if (argc == 1 || argc > 2) {
    fprintf(stderr, "Usage: file path");
    return EXIT_FAILURE;
  };

  char* file_path = argv[1];
  size_t num;
  

  FILE *f = fopen(file_path, "r");

  // 2.3 error if filepath or name doesnt exist
  if (!f) {
    print_error(file_path, errno);
    return EXIT_SUCCESS;
  }

  // get size of file 
  fseek(f, 0, SEEK_END);
  size_t buffer_size = ftell(f);
  char buffer[buffer_size + 1];
  rewind(f); //reset filestream to beginning of file
  //count number of read bytes
  num = fread(buffer, sizeof( char ), buffer_size, f);
  fclose(f);
  
  if (!num) {
    printf("%s: %s", file_path, FILE_TYPE_STRINGS[1]);
  } else {
    // DATA: TO BE CHANGED 
    printf("%s: %zu", file_path, num);
  }

  return EXIT_SUCCESS;
}
