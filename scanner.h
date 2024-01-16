#ifndef mu_scanner_h
#define mu_scanner_h

typedef enum { // TODO update to reflect mu
  // Single-character tokens.
  S_QUESTION, S_PLUS, S_MINUS, S_SLASH, S_STAR,
  S_COMMA, S_DOT, S_SEMICOLON, S_COLON,   
  S_LEFT_PARENTHESES, S_RIGHT_PARENTHESES,
  S_LEFT_CURLY, S_RIGHT_CURLY, // S_OCTO,
  S_LEFT_SQUARE, S_RIGHT_SQUARE, TOKEN_BANG, 
  S_EQUAL, TOKEN_GREATER, TOKEN_LESS,
  // two character tokens.
  D_COMMA,
  TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER_EQUAL, 
  TOKEN_LESS_EQUAL,
  // Literals.
  L_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER, 
  // L_MUTABLE, // todo add, would replace s_octo ?
  // Keywords.
  TOKEN_CLASS, 
  TOKEN_FOR, 
  TOKEN_NIL, 
  TOKEN_PRINT, 
  TOKEN_SUPER, 
  TOKEN_THIS,

  K_AND, 
  K_DEFINE,
  K_ELSE,
  K_FALSE,
  K_IF, 
  K_LET, // TODO remove 
  K_OR,
  K_RETURN, 
  K_UNTIL,
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
