#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source) {
  initScanner(source);

  //TODO replace after better implementation
  /* 
    almost certain could move token = scanToken to above and the bottom
    and make while (token.type != EOF)
  */
  int line = -1;
  for (;;) {
    Token token = scanToken();
    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    } else {
      printf(" | ");
    }
    printf("%2d '%.*s'\n", token.type, token.length, token.start);

    if (token.type == END_OF_FILE) break;
  }
}