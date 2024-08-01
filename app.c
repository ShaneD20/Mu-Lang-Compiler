#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "disassemble.h"
#include "vm.h"

/* 
  For tracking new lines, the scanner would tokenize them, 
  while the compiler would choose to keep or toss (or replace with semicolon) 
*/
static void repl() {
  char line[1024];
  for (;;) {
    printf("\n ~``~ ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interpret(line);
  }
}

static char* readFile(const char* path) {
  FILE* file = fopen(path, "rb");
  // handle no-file
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(fileSize + 1);
  // handle no-buffer
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }
  
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  // handle none-read
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

static void runFile(const char* path) {
  char* source = readFile(path);
  InterpretResult result = interpret(source);
  free(source); // [owner]

  switch (result) {
    case INTERPRET_COMPILE_ERROR: exit(65);
      break;
    case INTERPRET_RUNTIME_ERROR: exit(70);
      break;
    case INTERPRET_OK: // NO OP
      break;
  }
}

int main(int argc, const char* argv[]) {
  initVM();
  
  switch (argc) {
    case 1: repl();
      break;
    case 2: runFile(argv[1]);
      break;
    default:
      fprintf(stderr, "Usage: mu-lang [path]\n");
      exit(64);
  }
  freeVM();
  return 0;
}