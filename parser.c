#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "scanner.h"
#include "parser.h"

Parser parser;

void initParser() {}

// getters
Token currentToken() { return parser.head; }
Token secondToken() { return parser.previous; }
Token thirdToken() { return parser.caboose;}
Token fourthToken() { return parser.tail; }
bool hasError() { return parser.hasError; }
bool panic() { return parser.panicMode; }

// setters
void setCurrent(Token token) { parser.head = token; }
void stopPanic() { parser.panicMode = false; }

// error helpers
void parserError(bool hasError) {
    parser.hasError = hasError;
}

void errorAt(Token* token, const char* message) {
  if (parser.panicMode) { return; }

  parser.panicMode = true;
  fprintf(stderr, "\n[line %d] Error\n", token->line);

  if (token->lexeme == END_OF_FILE) {
    fprintf(stderr, " at end");
  } else if (token->lexeme == LANGUAGE_ERROR) { /* No procedures, pass through */
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
  errorAt(&parser.head, message);
}

// validation
bool tokenIs(Lexeme test) {        
  return parser.head.lexeme == test;
}
bool tokenIsNot(Lexeme test) {
  return parser.head.lexeme != test;
}
bool previousIsNot(Lexeme test) {
  return parser.previous.lexeme != test;
}

// traversal

void advance() {
  parser.tail = parser.caboose;
  parser.caboose = parser.previous;
  parser.previous = parser.head;
  parser.head = scanToken();

  if (parser.head.lexeme == LANGUAGE_ERROR) {
    Token token = parser.head;
    do {
      parser.head = scanToken();
    } while (parser.head.lexeme != LANGUAGE_ERROR);
    error(token.start); 
  }
}

bool consume(Lexeme glyph) {
  if (tokenIs(glyph)) advance();
  return parser.previous.lexeme == glyph;
}

void require(Lexeme test, const char* message) {
  if (parser.head.lexeme == test) {
    advance();
  } else error(message);
}

void synchronize() {
  stopPanic();
  Lexeme currentLexeme = currentToken().lexeme;

  while (currentLexeme != END_OF_FILE) {
    if (currentLexeme == S_SEMICOLON) {
      return;
    } 
    switch (currentLexeme) {
      case K_AS:
      case K_IF:
      case K_UNLESS:
      case K_WHEN:
      case K_UNTIL:
      case K_WHILE:
      case K_QUIT:
      case K_USE:
      case TOKEN_PRINT:
      case K_RETURN:
        return;
      default: ;
    }
    advance();
  }
}