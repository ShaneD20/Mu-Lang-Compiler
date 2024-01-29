#ifndef mu_scanner_h
#define mu_scanner_h

typedef enum { 
  // Single character tokens
  S_PLUS, S_MINUS, S_SLASH, S_STAR,         // + - / *
  S_QUESTION, S_EQUAL, S_GREATER, S_LESS,   // ? = > <
  S_COMMA, S_DOT, S_SEMICOLON, S_COLON,     // , . ; :
  S_LEFT_ROUND, S_RIGHT_ROUND,              // ( )
  S_LEFT_CURLY, S_RIGHT_CURLY, S_MODULO,    // { } %
  S_LEFT_SQUARE, S_RIGHT_SQUARE, S_BANG,    // [ ] !
  S_AMPERSAND, S_PIPE, S_RAISE, S_TILDE,    // & | ^ ~
  // Two character tokens
  D_COMMA, D_BANG_TILDE, D_COLON_EQUAL,     // ,, !~ :=
  D_GREATER_EQUAL, D_LESS_EQUAL, D_DOT,     // >= <= ..
  D_PLUS_EQUAL, D_STAR_EQUAL, D_DOT_EQUAL,  // += *= .=
  D_MODULO_EQUAL, D_SLASH_EQUAL,            // %= /= 
  //D_DIAMOND, // <>
  // Literals
  L_IDENTIFIER, L_MUTABLE, L_STRING, L_NUMBER, 
   // L_ARRAY,     // TODO implement array
  
  // Keywords
  K_AND, K_AS, K_IF, K_UNLESS, K_ELSE,       //  5
  K_WHEN, K_IS, K_OR, K_RETURN, K_USE,       // 10
  K_WHILE, K_TRUE, K_UNTIL, K_FALSE, K_NULL, // 15
  K_LET, K_QUIT,
  // currently experimental
  K_DEFINE, K_DO, TOKEN_PRINT, 
  // need to implement
  K_LIKE, // pattern matching
  K_TO,
  // NEW_LINE, // TODO test
  LANGUAGE_ERROR, END_OF_FILE
} Lexeme;

// TODO experimental compiler type-checking
typedef enum {
  VT_FUNCTION,
  VT_INDEX_COLLECTION,
  VT_KEYED_COLLECTION,
  VT_MATH,
  VT_NAME,
  VT_TEXT,
  VT_VOID,
} VariableType; 
//^ TODO find a better name 

typedef struct {
  VariableType type;
  Lexeme lexeme;
  const char* start; 
  /* add a compiler table to store, tail name on initialization */
  /* stores equivalent VariableType */
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
