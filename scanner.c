
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

static char peekTwoAhead() {
  if (isAtEnd()) return '\0'; 
  return scanner.current[2];
}

static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++; // TODO would NEED to implement something similar for meaningful new lines
  return true;
}
//^ Scanner Helpers

//> Token Helpers
static Token makeToken(Lexeme lexeme, VariableType type) {
  Token token;
  token.type = type;
  token.lexeme = lexeme;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token errorToken(const char* message) {
  Token token;
  token.lexeme = LANGUAGE_ERROR;
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

static Lexeme checkKeyword(int start, int length, const char* rest, Lexeme lexeme) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return lexeme;
  }
  return L_IDENTIFIER;
}

static Lexeme identifierType() { // tests for keywords
  switch (scanner.start[0]) {
    case 'a': //branch our to "and", "as"
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'n' : return checkKeyword(2, 1, "d", K_AND); 
          case 's' : return checkKeyword(2, 0, "", K_AS);
        }
      }
      break;
    case 'd': 
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'e' : return checkKeyword(2, 4, "fine", K_DEFINE);
          case 'o' : return checkKeyword(2, 0, "", K_DO);
        }
      }
      break;
    case 'e': // branch out to "else",
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'l' : return checkKeyword(2, 2, "se", K_ELSE);
        }
      }
      break;
    case 'f': return checkKeyword(1, 4, "alse", K_FALSE);
    case 'i': // branch out to 'if', 'is'
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'f': return checkKeyword(2, 0, "", K_IF);
          case 's': return checkKeyword(2, 0, "", K_IS);
        }
      }
      break;
    case 'l': return checkKeyword(1, 2, "et", K_LET);
    case 'n': return checkKeyword(1, 3, "ull", K_NULL); // because JSON uses 'null'
    case 'o': return checkKeyword(1, 1, "r", K_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT); // TODO replace with native function
    case 'r': return checkKeyword(1, 5, "eturn", K_RETURN);
    case 't': // branch out to "to", "true"
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'o': return checkKeyword(2, 0, "", K_TO);
          case 'r': return checkKeyword(2, 2, "ue", K_TRUE);
        }
      }
      break;
    case 'u': // if 'n' then branch out to 'use', 'unless', 'until'
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 's': return checkKeyword(2, 1, "e", K_USE);
          case 'n': switch (scanner.start[2]) {
            case 'l': return checkKeyword(3, 3, "ess", K_UNLESS);
            case 't': return checkKeyword(3, 2, "il", K_UNTIL);
          }
        }
      }
      break;
    case 'w': // if 'h' then branch out to 'when', 'while'
      if (scanner.current - scanner.start > 1 && scanner.start[1] == 'h') {
        switch (scanner.start[2]) {
          case 'e': return checkKeyword(3, 1, "n", K_WHEN);
          case 'i': return checkKeyword(3, 2, "le", K_WHILE);
        }
      }
      break;
    case 'q': return checkKeyword(1, 3, "uit", K_QUIT);
    case '#': return L_MUTABLE;
  }
  return L_IDENTIFIER;
}

static Token identifier() {
  if (peek() == '#') {
    advance();
  }
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType(), VT_NAME);
}

static Token number() {
  while (isDigit(peek())) advance();

  if (peek() == '.' && isDigit(peekNext())) { // Look for a fractional part.
    advance(); // Consume "."

    while (isDigit(peek())) advance();
  }
  return makeToken(L_NUMBER, VT_MATH);
}

static Token string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') scanner.line++;
    advance();
  }
  if (isAtEnd()) return errorToken("Unterminated string.");

  advance(); // The closing quote.
  return makeToken(L_STRING, VT_INDEX_COLLECTION); // TODO may need to add string type instead
}

Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;
  if (isAtEnd()) return makeToken(END_OF_FILE, VT_VOID);

  // check for constants
  char rune = advance();
  if (isAlpha(rune)) return identifier(); 
  if (isDigit(rune)) return number(); 

  switch (rune) {
    // single character
    case '#': return identifier();
    case '(': return makeToken(S_LEFT_ROUND, VT_VOID); // TODO would be new line aware
    case ')': return makeToken(S_RIGHT_ROUND, VT_VOID);
    case '{': return makeToken(S_LEFT_CURLY, VT_VOID);   // TODO would be new line aware
    case '}': return makeToken(S_RIGHT_CURLY, VT_VOID);
    case '?': return makeToken(S_QUESTION, VT_VOID);     // TODO would be new line aware
    case ';': return makeToken(S_SEMICOLON, VT_VOID);
    case '=': return makeToken(S_EQUAL, VT_VOID); 
    case '-': return makeToken(S_MINUS, VT_VOID);
    case '&': return makeToken(S_AMPERSAND, VT_VOID);
    case '|': return makeToken(S_PIPE, VT_VOID);
    case '^': return makeToken(S_RAISE, VT_VOID);
    case '~': return makeToken(S_TILDE, VT_VOID);
    // two characters
    case '.': 
      switch(scanner.start[1]) {
        case '=': advance();
          return  makeToken(D_DOT_EQUAL, VT_VOID);
        case '.': advance();
          return makeToken(D_DOT, VT_VOID);
        default: 
          return makeToken(S_DOT, VT_VOID);
      }
      break;
    case ',': 
      return makeToken(match(',') ? D_COMMA : S_COMMA, VT_VOID);
    case '%': 
      return makeToken(match('=') ? D_MODULO_EQUAL : S_MODULO, VT_VOID);
    case '+': 
      return makeToken(match('=') ? D_PLUS_EQUAL : S_PLUS, VT_VOID);
    case '/': 
      return makeToken(match('=') ? D_SLASH_EQUAL : S_SLASH, VT_VOID);
    case '*': 
      return makeToken(match('=') ? D_STAR_EQUAL : S_STAR, VT_VOID);
    case ':': 
      return makeToken(match('=') ? D_COLON_EQUAL: S_COLON, VT_VOID);
    case '<':
      return makeToken(match('=') ? D_LESS_EQUAL : S_LESS, VT_VOID);
    case '>':
      return makeToken(match('=') ? D_GREATER_EQUAL : S_GREATER, VT_VOID);
    case '!':
      return makeToken(match('~') ? D_BANG_TILDE : S_BANG, VT_VOID);
    case '"': return string();
  }
  return errorToken("Unexpected character.");
}
