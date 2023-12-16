#include <stdio.h>
#include <string.h>
#include "common.h"
#include "scanner.h"

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

Token scanToken() { //used in compiler.c
  handleWhitespace();
  scanner.start = scanner.current;

  if (isAtEnd()) {
    return tokenize(END_OF_FILE);
  }
  char rune = advance();

  if (isDigit(rune)) {
    return numberize();
  }

  switch (rune) {
    // single character
    case '(' : return tokenize(S_LEFT_PARENTHESIS);
    case '[' : return tokenize(S_LEFT_SQUAREBRACE);
    case '{' : return tokenize(S_LEFT_CURLYBRACE);
    case ')' : return tokenize(S_RIGHT_PARENTHESIS);
    case ']' : return tokenize(S_RIGHT_SQUARE_BRACE);
    case '}' : return tokenize(S_RIGHT_CURLYBRACE);
    case ';' : return tokenize(S_SEMICOLON);
    case ',' : return tokenize(S_COMMA);
    case '.' : return tokenize(S_DOT);
    case '-' : return tokenize(S_MINUS);
    case '+' : return tokenize(S_PLUS);
    case '*' : return tokenize(S_STAR);
    case '/' : return tokenize(S_SLASH);
    // double character
    case '!' : return tokenize(match('~') ? D_NOT_EQUAL : S_BANG);
    case '=' : return tokenize(match('=') ? D_EQUAL : S_EQUAL);
    case '<' : return tokenize(match('=') ? D_LESS_EQUAL : S_LESS);
    case '>' : return tokenize(match('=') ? D_GREATER_EQUAL : S_GREATER);
    //literal
    case '"' : return string();

  }

  return errorToken("unexpected character");

}
static char advance() {
  scanner.current += 1;
  return scanner.current[-1];
}
static char peek() {
  return *scanner.current;
}
static char peekNext() {
  if (isAtEnd()) {
    return '\0';
  }
  return scanner.current[1];
}
static bool isAtEnd() {
  return *scanner.current == '\0';
}
static bool match(char expected) {
  if (isAtEnd() || *scanner.current != expected) { //TODO 
    return false;
  }
  scanner.current += 1;
  return true;
}
static bool isDigit(char rune) {
  return rune >= '0' && rune <= '9';
}
static Token numberize() {
  while (isDigit(peek())) {
    advance();
  }
  if (peek() == '.' && isDigit(peekNext())) {
    advance(); //consumes '.'
    while (isDigit(peek())) {
    advance();
    }
  }
}
static Token tokenize(TokenType type) { // can see how this could easily be non-exhaustive
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}
static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}
static void handleWhitespace() {
  for (;;) {
    char test = peek();

    switch (test) {
      case ' ' :
      case '\r':
      case '\t': 
        advance();
        break;
      case '\n':
        advance();
        scanner.line += 1;
        break;
      case ';' :
        if (peekNext() == ';') {
          while (peek() != '\n' && !isAtEnd()) {
            advance();
          }
        } else {
          return;
        }
        break; 
      default : 
        return;
    }
  }
}

static Token string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') {
      scanner.line += 1;
    } 
    advance();
  }
  if (isAtEnd()) {
    return errorToken("Unterminated String.");
  }
  advance();
  return tokenize(L_STRING);
}