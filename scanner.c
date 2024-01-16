#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

Scanner scanner;

void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}
//> Scanner Helpers
static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isAtEnd() {
  return *scanner.current == '\0';
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static char peek() {
  return *scanner.current;
}

static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++; // TODO would implement something similar for meaningful new lines
  return true;
}
//^ Scanner Helpers

//> Token Helpers
static Token makeToken(TokenType type) {
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

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ': 
      case '\r':
      case '\t':
        advance();
        break;
      case '\n': // newline
        scanner.line++;
        advance();
        break;
      case '/':
        if (peekNext() == '/') { // comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;
      default: //encountering anything not defined as whitespace
        return;
    }
  }
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return L_IDENTIFIER; // TODO create constant, mutable path
}

static TokenType identifierType() { // tests for keywords
  switch (scanner.start[0]) {
    case 'a': return checkKeyword(1, 2, "nd", K_AND);
    case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(1, 3, "lse", K_ELSE);
    case 'd': return checkKeyword(1, 5, "efine", K_DEFINE);
//> keyword-f
    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'a': return checkKeyword(2, 3, "lse", K_FALSE);
          case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
        }
      }
      break;
//< keyword-f
    case 'i': return checkKeyword(1, 1, "f", K_IF);
    case 'l': return checkKeyword(1, 2, "et", K_LET);
    case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o': return checkKeyword(1, 1, "r", K_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", K_RETURN);
    case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
//> keyword-t
    case 't':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
          case 'r': return checkKeyword(2, 2, "ue", K_TRUE);
        }
      }
      break;
//< keyword-t
    case 'u': return checkKeyword(1, 4, "ntil", K_UNTIL);
    case 'w': return checkKeyword(1, 4, "hile", K_WHILE);
  }

//< keywords
  return L_IDENTIFIER; // TODO add mutable path
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}

static Token number() {
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    advance(); // Consume the "."

    while (isDigit(peek())) advance();
  }
  return makeToken(TOKEN_NUMBER);
}

static Token string() {
  while (peek() != '"' && !isAtEnd()) { // TODO use similar logic if implementing meaningufl new lines
    if (peek() == '\n') scanner.line++;
    advance();
  }
  if (isAtEnd()) return errorToken("Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}
//^ Token Helpers

Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;
  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c)) return identifier(); // check for identifier
  if (isDigit(c)) return number(); // check for number

  switch (c) {
    // single character
    case '(': return makeToken(S_LEFT_PARENTHESES); // TODO would be new line aware
    case ')': return makeToken(S_RIGHT_PARENTHESES);
    case '{': return makeToken(S_LEFT_CURLY); // TODO would be new line aware
    case '}': return makeToken(S_RIGHT_CURLY);
    case '?': return makeToken(S_QUESTION); // TODO would be new line aware
    case ';': return makeToken(S_SEMICOLON);
    case ':': return makeToken(S_COLON); // TODO would be new line aware
    case '.': return makeToken(S_DOT);
    case '-': return makeToken(S_MINUS);
    case '+': return makeToken(S_PLUS);
    case '/': return makeToken(S_SLASH);
    case '*': return makeToken(S_STAR);
    // two characters
    case ',': 
      return makeToken(match(',') ? D_COMMA : S_COMMA);
    case '!':
      return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : S_EQUAL);
    case '<':
      return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(
        match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    // strings
    case '"': return string();
  }
  return errorToken("Unexpected character.");
}
