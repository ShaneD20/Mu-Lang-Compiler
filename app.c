#include <stdio.h>
#include <stdlib.h> // .h implies header file, provides an interface to the module
#include <string.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

// top level pointer variables --> iVariable
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

static char* readFile(const char* o_path) {
  FILE* o_file = fopen(o_path, "rb");
  // handle no file
  if (o_file == NULL) { 
    fprintf(stderr, "Could not open file \"%s\".'n", o_path);
    exit(74);
  }

  fseek(o_file, 0L, SEEK_END);
  size_t fileSize = ftell(o_file);
  rewind(o_file);

  char* o_buffer = (char*)malloc(fileSize + 1);
  //handle no buffer
  if (o_buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", o_path);
    exit(74);
  }

  size_t bytesRead = fread(o_buffer, sizeof(char), fileSize, o_file);
  o_buffer[bytesRead] = '\0';

  fclose(o_file);
  return o_buffer;
}

static void runFile(const char* o_path) {
  char* o_source = readFile(o_path);
  InterpretResult result = interpret(o_source);
  free(o_source);

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
