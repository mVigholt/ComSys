#include <stdio.h>  // fprintf, stdout, stderr.
#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS.
#include <string.h> // strerror.
#include <errno.h>  // errno.
#include <strings.h> //string type
#include <stdbool.h> // bool


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
  "ISO-8859 text",
  "Unicode text, UTF-8 text"
};

// Assumes: errnum is a valid error number
int print_error(char *path, int errnum) {
  return fprintf(stdout, "%s: cannot determine (%s)\n",
    path, strerror(errnum));
}

int main(int argc, char* argv[argc+1]) {

  // 2.2 return stderr if no input path or too many arguments
  if (!(argc == 2)) {
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

  //load file to streamreader
  //all chars are stored in the buffer array
  //see ref: https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-fread-read-items
  fseek(f, 0, SEEK_END);
  size_t buffer_size = ftell(f);
  char buffer[buffer_size + 1];
  rewind(f); //reset filestream to beginning of file
  num = fread(buffer, sizeof( char ), buffer_size, f); //count number of read bytes
  fclose(f);

  unsigned type = EMPTY;

  bool isUtf = true;
  bool isIso = true;
  bool isAsci = true;

  unsigned char n = 0;  //For the last read byte
  unsigned char k = 0;  //How many extra bytes are expected in the current UTF sequence

  if (!num) {
    type = EMPTY;
  } else {
    // DATA/ASCII/ISO/UTF8
    for (size_t i = 0; i < buffer_size; i++) {
      n = buffer[i];

      if (!(((n >= 7) && (n <= 13)) || (n == 27) || ((n >= 32) && (n <= 126)))) {
        if (isAsci) {
          isAsci = false;
        }

        if (isIso && (!(n >= 160))){
          isIso = false;
        }
      }

      if (isUtf) {
        if ((n < 7) || (n > 247)) {
          isUtf = false;  
        } 
        
        if (k == 0) {
          if ((n >= 128) && (n <= 191)) { //10xxxxxx
            isUtf = false;
          }
          if ((n >= 192) && (n <= 223)) { //110xxxxx
            k = 1;
          }
          if ((n >= 224) && (n <= 239)) { //1110xxxx
            k = 2;
          }
          if ((n >= 240) && (n <= 247)) { //11110xxx
            k = 3;
          }
        } else {
          if ((n >= 128) && (n <= 191)) { //10xxxxxx
            k = k - 1;
          } else {
            isUtf = false;
          }
        }
      }
    }

    if (isAsci) {
      type = ASCII;
    } else if (isIso) {
      type = ISO8859;
    } else if (isUtf && (k == 0)) {
      type = UTF8;
    } else {
      type = DATA;
    }
  }

  fprintf(stdout, "%s: %s\n", file_path, FILE_TYPE_STRINGS[type]);

  return EXIT_SUCCESS;
}
