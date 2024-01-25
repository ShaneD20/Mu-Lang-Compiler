#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "scanner.h"
#include "parser.h"

Parser parser;

Token previousToken() {
    return parser.previous;
}
Token parserCurrent() {
    return parser.current;
}
void setCurrent(Token token) {
    parser.current = token;
}
void parserError(bool hasError) {
    parser.hasError = hasError;
}
bool hasError() {
    return parser.hasError;
}
bool panic() {
    return parser.panicMode;
}
void stopPanic() {
    parser.panicMode = false;
}

void errorAt(Token* token, const char* message) {
  if (parser.panicMode) {
    return;
  } 
  parser.panicMode = true;
  fprintf(stderr, "\n[line %d] Error\n", token->line);

  if (token->lexeme == END_OF_FILE) {
    fprintf(stderr, " at end");
  } else if (token->lexeme == LANGUAGE_ERROR) { 
    // Nothing, pass through
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hasError = true;
  exit(1);
}

void error(const char* message) {
  errorAt(&parser.previous, message);
}
void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

void advance() {
  parser.previous = parser.current;
  parser.current = scanToken();

  if (parser.current.lexeme == LANGUAGE_ERROR) {
    Token token = parser.current;
    do {
      parser.current = scanToken();
    } while (parser.current.lexeme != LANGUAGE_ERROR);
    error(token.start); 
  }
}

bool tokenIs(Lexeme test) {        
  return parser.current.lexeme == test;
}
bool tokenIsNot(Lexeme test) {
  return parser.current.lexeme != test;
}
bool previousIsNot(Lexeme test) {
  return parser.previous.lexeme != test;
}

bool consume(Lexeme glyph) {
  if (tokenIs(glyph)) advance();
  return parser.previous.lexeme == glyph;
}

void require(Lexeme test, const char* message) {
  if (parser.current.lexeme == test) {
    advance();
  } else error(message);
}
