#ifndef mu_scanner_h
#define mu_scanner_h

typedef enum { // TODO update to reflect mu
  // Single-character tokens.
  S_PLUS, S_MINUS, S_SLASH, S_STAR,
  S_COMMA, S_DOT, S_SEMICOLON, S_COLON,   
  S_LEFT_PARENTHESES, S_RIGHT_PARENTHESES,
  S_LEFT_CURLY, S_RIGHT_CURLY, S_MODULO,
  S_LEFT_SQUARE, S_RIGHT_SQUARE, S_BANG, 
  S_EQUAL, S_GREATER, S_LESS, S_QUESTION,
  // two character tokens.
  D_COMMA, D_BANG_TILDE, D_COLON_EQUAL,
  D_GREATER_EQUAL, D_LESS_EQUAL,
  D_PLUS_EQUAL,
  // Literals.
  L_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER, L_VARIABLE,
  // L_MUTABLE, // todo add, would replace s_octo ?
  // Keywords.
  // TOKEN_FOR, 
  TOKEN_CLASS, 
  TOKEN_NIL, 
  TOKEN_PRINT, 
  TOKEN_SUPER, // could use 'bind' to bind a global to a closure

  K_AND, 
  K_DEFINE,
  K_ELSE,
  K_FALSE,
  // would 'go' work in place of 'end' ?
  K_IF, K_IS,
  K_LET, // TODO remove 
  K_OR,
  K_RETURN, 
  K_SELF,
  K_UNLESS, K_UNTIL,
  K_WHEN,
  K_WHILE,
  K_TRUE, 
  NEW_LINE, // TODO test
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
