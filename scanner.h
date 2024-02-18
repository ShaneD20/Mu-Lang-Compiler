#ifndef mu_scanner_h
#define mu_scanner_h

typedef enum { 
  // Single character tokens
  S_PLUS, S_MINUS, S_SLASH, S_STAR,         // + - / *
  S_QUESTION, S_EQUAL, S_GREATER, S_LESS,   // ? = > <
  S_COMMA, S_DOT, S_SEMICOLON, S_COLON,     // , . ; :
  SL_ROUND, SL_CURLY, SL_SQUARE, S_BANG,    // ( { [ !
  SR_ROUND, SR_CURLY, SR_SQUARE, S_MODULO,  // ) } ] %
  S_AMPERSAND, S_PIPE, S_RAISE, S_TILDE,    // & | ^ ~

  // Two character tokens
  D_COMMA, D_BANG_TILDE, D_COLON_EQUAL,     // ,, !~ :=
  D_GREATER_EQUAL, D_MODULO_EQUAL, D_DOT,   // >= %= ..
  D_PLUS_EQUAL, D_STAR_EQUAL, D_DOT_EQUAL,  // += *= .=
  D_LESS_EQUAL, D_SLASH_EQUAL, D_STAR_L_ROUND,  // <= /= *( 
  D_STAR_R_ROUND,

  // Literals
  L_IDENTIFIER, L_MUTABLE, L_STRING, L_NUMBER, 
  // L_ARRAY,     // TODO implement array
  
  // Keywords
  K_AND, K_AS, 
  K_ELSE,       
  K_FALSE, 
  // K_FROM,
  // K_GIVE,
  // K_HAS,
  K_IF, K_IS,
  K_NULL,
  K_OR,
  K_QUIT,
  K_RETURN,
  K_TRUE, 
  K_WHEN, K_WHILE, 
  K_UNLESS, K_UNTIL, K_USE,   // 16 active     
  // currently experimental
  K_DEFINE, TOKEN_PRINT, K_NOT,
  K_TO, K_LIKE,
  // NEW_LINE,
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
