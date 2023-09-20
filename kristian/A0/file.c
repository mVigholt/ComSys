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

// Assumes: errnum is a valid error number
int print_error(char *path, int errnum) {
  return fprintf(stdout, "%s: cannot open: %s\n",
    path, strerror(errnum));
}

int main(int argc, char* argv[argc+1]) {

  // 2.2 return stderr if no input path or too many arguments
  if (argc == 1 || argc > 2) {
    fprintf(stderr, "Usage: file path\n");
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
  //all chars are stored in the buffer array
  num = fread(buffer, sizeof( char ), buffer_size, f);
  fclose(f);

  unsigned type = 0;

  if (!num) {
    //empty file
    type = 1;
  } else {
    // DATA/ASCII
    // convert char to hex or binary and check if it exists in the union set of ASCII, if it does continue looping through buffer array
    // if it doesnt, break loop and return data identification
    for (size_t i = 0; i < buffer_size; i++) {
      unsigned decimal = buffer[i];
      //fprintf(stdout, "%u\n", decimal);
      if ((decimal > 6 && decimal < 15) || decimal == 28 || (decimal >= 32 && decimal < 128)) {
        type = 2;
      } else {
        type = 0;
        break;
      };
    }
  }

  //types: 0=data, 1=empty, 2=ascii
  fprintf(stdout, "%s: %s\n", file_path, FILE_TYPE_STRINGS[type]);

  return EXIT_SUCCESS;
}
