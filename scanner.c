#include <stdio.h>
#include <string.h>
#include "common.h"
#include "scanner.h"

typedef struct {
  const char* start_pointer;
  const char* current_pointer;
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* iSource) {
  scanner.start_pointer = iSource;
  scanner.current_pointer = iSource;
  scanner.line = 1;
}

/* start HELPER functions
*/
static bool isAtEnd() {
  return *scanner.current_pointer == '\0';
}
static char advance() {
  scanner.current_pointer += 1;
  return scanner.current_pointer[-1];
}
static char peek() {
  return *scanner.current_pointer;
}
static char peekNext() {
  if (isAtEnd()) {
    return '\0';
  }
  return scanner.current_pointer[1];
}

static bool match(char expected) {
  if (isAtEnd() || *scanner.current_pointer != expected) { //TODO 
    return false;
  }
  scanner.current_pointer += 1;
  return true;
}
static bool isAlpha(char rune) {
  return (rune >= 'a' && rune <= 'z') ||
         (rune >= 'A' && rune <= 'Z') ||
          rune == '_';
}
static bool isDigit(char rune) {
  return rune >= '0' && rune <= '9';
}
static Token tokenize(TokenType type) { // can see how this could easily be non-exhaustive
  Token token;
  token.type = type;
  token.start_pointer = scanner.start_pointer;
  token.length = (int)(scanner.current_pointer - scanner.start_pointer);
  token.line = scanner.line;
  return token;
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
  return tokenize(L_FLOAT);
}
static Token errorToken(const char* iMessage) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start_pointer = iMessage;
  token.length = (int)strlen(iMessage);
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
/* end HELPER functions
*/
static TokenType testKeyword(int start, int length, const char* iRemaining, TokenType token) {
  if (scanner.current_pointer - scanner.start_pointer == start + length
      && memcmp(scanner.start_pointer + start, iRemaining, length) == 0) {
        return token;
  }
  return L_IDENTIFIER;
}
static TokenType KeywordOrLiteral() {
  switch (scanner.start_pointer[0]) {
    case 'a' : return testKeyword(1, 2, "nd", K_AND);
    case 'b' : return testKeyword(1, 4, "uild", K_BUILD);
    case 'd' : return testKeyword(1, 5, "efine", K_DEFINE);
    case 'e' : return testKeyword(1, 3, "lse", K_ELSE);
    case 'f' : return testKeyword(1, 4, "alse", K_FALSE);
    case 'i' : return testKeyword(1, 1, "f", K_IF);
    case 'j' : return testKeyword(1, 3, "oin", K_JOIN);
    case 'n' : return testKeyword(1, 2, "ot", K_NOT);
    case 'o' : return testKeyword(1, 1, "r", K_OR);
    case 'r' : return testKeyword(1, 5, "eturn", K_RETURN);
    case 's' : return testKeyword(1, 3, "elf", K_SELF);
    case 't' : return testKeyword(1, 4, "rue", K_TRUE);
    case 'u' : if (scanner.current_pointer - scanner.start_pointer > 1 && scanner.start_pointer[1] == 'n') {
      switch (scanner.start_pointer[2]) {
        case 'l' : return testKeyword(1, 3, "ess", K_UNLESS);
        case 't' : return testKeyword(1, 2, "il", K_UNTIL);
      }
    }
    case 'v' : return testKeyword(1, 3, "oid", K_VOID);
    case 'w' : return testKeyword(1, 4, "hile", K_WHILE);
    case 'x' : return testKeyword(1, 2, "or", K_XOR);
    // case 'y' : return testKeyword(1, 4, "ield", K_YIELD);
  }
  return L_IDENTIFIER;
}
static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) {
    advance();
  }
  return tokenize(KeywordOrLiteral());
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

Token scanToken() { //used in compiler.c
  handleWhitespace();
  scanner.start_pointer = scanner.current_pointer;

  if (isAtEnd()) {
    return tokenize(END_OF_FILE);
  }
  char rune = advance();

  if (isAlpha(rune)) {
    return identifier();
  }

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
    case ':' : return tokenize(S_COLON);
    case ';' : return tokenize(S_SEMICOLON);
    case ',' : return tokenize(S_COMMA);
    case '.' : return tokenize(S_DOT);
    case '-' : return tokenize(S_MINUS);
    case '+' : return tokenize(S_PLUS);
    case '*' : return tokenize(S_STAR);
    case '/' : return tokenize(S_SLASH);
    case '#' : return tokenize(S_OCTO);
    // double character
    case '!' : return tokenize(match('~') ? D_BANG_TILDE : S_BANG);
    case '=' : return tokenize(match('=') ? D_EQUAL : S_EQUAL);
    case '<' : return tokenize(match('=') ? D_LESS_EQUAL : S_LESS);
    case '>' : return tokenize(match('=') ? D_GREATER_EQUAL : S_GREATER);
    //literal
    case '"' : return string();
  }
  return errorToken("unexpected character");
}

