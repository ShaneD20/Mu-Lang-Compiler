#ifndef mu_scanner_h
#define mu_scanner_h

typedef enum { // TODO keep updating to match mu
  // Single-character tokens.
  S_PLUS, S_MINUS, S_SLASH, S_STAR,         // + - / *
  S_QUESTION, S_EQUAL, S_GREATER, S_LESS,   // ? = > <
  S_COMMA, S_DOT, S_SEMICOLON, S_COLON,     // , . ; :
  S_LEFT_PARENTHESES, S_RIGHT_PARENTHESES,  // ( )
  S_LEFT_CURLY, S_RIGHT_CURLY, S_MODULO,    // { } %
  S_LEFT_SQUARE, S_RIGHT_SQUARE, S_BANG,    // [ ] !
  // two character tokens.
  D_COMMA, D_BANG_TILDE, D_COLON_EQUAL,     // ,, !~ :=
  D_GREATER_EQUAL, D_LESS_EQUAL,            // >= <=
  D_DIAMOND, D_PLUS_EQUAL, D_STAR_EQUAL,    // <> += *=
  // Literals.
  L_IDENTIFIER, L_STRING, L_NUMBER, L_VARIABLE, 
  // Keywords.
  // could use 'bind' to bind a closure 
  TOKEN_PRINT, 

  K_AND, K_AS,
  K_DEFINE,
  K_ELSE, K_END,
  K_FALSE,
  K_IF, K_IS,
  K_LET, // TODO consider removing
  K_NULL,
  K_OR,
  K_RETURN,
  K_SELF,
  K_UNLESS, K_UNTIL,
  K_WHEN, K_WHILE,
  K_TRUE, K_TO,
  // NEW_LINE, // TODO test
  TOKEN_ERROR, END_OF_FILE
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
