#ifndef mu_scanner_h
#define mu_scanner_h

typedef enum { // TODO update to reflect mu
  // Single-character tokens.
  S_QUESTION,
  S_LEFT_PARENTHESES, S_RIGHT_PARENTHESES,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
  // One or two character tokens.
  D_COMMA,
  TOKEN_BANG, TOKEN_EQUAL, TOKEN_GREATER,  
  TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER_EQUAL, TOKEN_LESS,
  TOKEN_LESS_EQUAL,
  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  // Keywords.
  TOKEN_CLASS, 
  TOKEN_FOR, 
  TOKEN_NIL, 
  TOKEN_PRINT, 
  TOKEN_SUPER, 
  TOKEN_THIS,
  TOKEN_VAR, 

  K_AND, 
  K_DEFINE, 
  K_ELSE,
  K_FALSE,
  K_IF, 
  K_OR,
  K_RETURN, 
  K_WHILE,
  K_TRUE, 

  TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct {
  TokenType type;
  const char* start;
  int length;
  int line;
} Token;

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

void initScanner(const char* source);
Token scanToken();

#endif
