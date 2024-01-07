#include <stdio.h>
#include <stdlib.h> // .h implies header file, provides an interface to the module
#include <string.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

// top level pointer variables --> iVariable | oVariable
// struct property pointers --> property_pointer

static void repl() {
  char line[1024];
  // do {
  //   printf(":: ");
  //   interpret(line);
  //   printf("\n");
  // } while (fgets(line, sizeof(line), stdin)); 

  for (;;) {
    printf(":: ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interpret(line); // vm.h
  }
}

static char* readFile(const char* oPath) {
  FILE* oFile = fopen(oPath, "rb");
  // handle no file
  if (oFile == NULL) { 
    fprintf(stderr, "Could not open file \"%s\".'n", oPath);
    exit(74);
  }

  fseek(oFile, 0L, SEEK_END);
  size_t fileSize = ftell(oFile);
  rewind(oFile);

  char* iBuffer = (char*)malloc(fileSize + 1);
  //handle no buffer
  if (iBuffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", oPath);
    exit(74);
  }

  size_t bytesRead = fread(iBuffer, sizeof(char), fileSize, oFile);
  iBuffer[bytesRead] = '\0';

  fclose(oFile);
  return iBuffer;
}

static void runFile(const char* oPath) {
  char* oSource = readFile(oPath);
  InterpretResult result = interpret(oSource);
  free(oSource);

  if (result == INTERPRET_COMPILE_ERROR) {
    exit(65);
  }
  if (result == INTERPRET_RUNTIME_ERROR) {
    exit(70);
  }
}

int main(int argc, const char* argv[]) {
  initVM();
  
  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: language [path]\n");
    exit(64);
  }

  freeVM();
  return EXIT_SUCCESS;
}
