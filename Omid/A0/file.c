#include <stdio.h>  // fprintf, stdout, stderr.
#include <stdlib.h> // exit, EXIT_FAILURE, EXIT_SUCCESS.
#include <string.h> // strerror.
#include <errno.h>  // errno.


int print_error(char *path, int errnum) {
  return fprintf(stdout, "%s: cannot determine (%s)\n", path, strerror(errnum));
}

int main(int argc, char* argv[]) {
  if (argc !=2){
    printf("Usage: %s file path\n", argv[0]);
    return 1;
  }
  //FILE er en struktur, der bruges til at repræsentere filer i C-programmering. 
  FILE *f = fopen(argv[1], "r");//Ved at erklære f som en pointer til en FILE, forbereder du dig på at åbne en fil og arbejde med den i dit program.
  if (f == NULL){//tejkke om filåbningen er mislykkedes eller der er ok.
    print_error(argv[1] , errno);
    return 0;
  }
  unsigned char c;// erklæring om at bruge c som en sted som gemme tingen ind.
  int asciiFound = 1;
  int isempty=1;
  int isISO= 1;
  
  while (fread(&c, sizeof(char), 1, f) == 1) {//funktionen til at læse en enkelt byte (char) fra filen f og gemme den i variablen c 

    isempty=0;
    if ((c >= 7 && c <= 13) || (c == 27 )|| (c >= 32 && c <= 126)){

    } else { 
      asciiFound =0;

    }
    if (!((c >= 7 && c <= 13) || (c == 27 )|| (c >= 32 && c <= 126) || (c >=160 && c <= 255)) ){
     isISO = 0;
     break;
    }
    

  }
  int i;
  int isUtf=1;
  int followerBytes;
  while (fread(&c, sizeof(char), 1, f) == 1) {
    if (c >> 7 == 0) {//hvis vi flytte 7 karakter til højer, så får vi 0 og det er UTF_8.
      continue;
    }

    // int utOne = 0xE >> 5;// hvis vi flytte 5 karakter til højer så få vi 1110 eller 6 tal.
    // int utfTow = 0x80 >> 6; // 
      if (c >> 5 ==  6){followerBytes =1;
      }
      
    // int utfCasTow = 0xE >> 4;
    // int utfCastow = 0x80 >> 6;
    // int utfTree = 0x80 >> 6;
      if (c >> 4 ==  14) {followerBytes =2;
      }
      
    // int utfCastree = 0x1E >> 5;
    // int utfCasTreTow = 0x80 >> 6;
    // int utfTreeTre = 0x80 >> 6;
    // int utfTreeTreFor = 0x80 >> 6;
      if (c >> 3 ==  30){followerBytes =3;
      } 

    for (i = 0; i < followerBytes ; i= i ++){
      if (fread(&c, sizeof(char), 1, f) == 1){
        if (c >> 6 == 2){
          continue;
        }
        break;
        
      }
    }
  }
 
  if(isempty==1){
    printf("%s: empty\n", argv[1]);
    return 0;

  } else if (asciiFound == 1){
    printf ("%s: ASCII text\n", argv[1]);
    return 0;

  } else if (isISO == 1){
    printf("%s: ISO-8859 text\n", argv[1]);
    return 0;
    
  } else if (isUtf == 1){
    printf("%s: Unicode text, UTF-8 text\n", argv[1]);
    return 0;
  }else {
    printf ("%s: data\n", argv[1]);
  }
  


}
  