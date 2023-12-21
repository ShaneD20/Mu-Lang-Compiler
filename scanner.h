#ifndef mu_scanner_h
#define mu_scanner_h

typedef enum {
  END_OF_FILE, TOKEN_ERROR,

  //SINGLE CHARACTER
  S_GRAVE, S_TILDE, S_BANG, S_AT, S_OCTO,    // ` ~ ! @ #
  S_MODULO, S_RAISE, S_AMPERSAND, S_STAR,    // % ^ & *
  S_LEFT_PARENTHESIS, S_RIGHT_PARENTHESIS,   // ( )
  S_MINUS, S_UNDERSCORE, S_PLUS, S_EQUAL,    // - _ + =
  S_LEFT_CURLYBRACE, S_RIGHT_CURLYBRACE,     // { }
  S_LEFT_SQUAREBRACE, S_RIGHT_SQUARE_BRACE,  // [ ]
  S_PIPE, S_BACKSLASH, S_COLON, S_SEMICOLON, // | \ : ; 
  S_QUOTE, S_APOSTROPHE, S_COMMA, S_DOT,     // " ' , .
  S_SLASH, S_LESS, S_GREATER, S_QUESTION,    // / < > ? 

  //DOUBLE CHARACTER
  D_COLON, D_DOT, D_EQUAL, D_QUESTION,        // :: .. == ?? 
  D_BANG_TILDE, D_GREATER_EQUAL, D_LESS_EQUAL, // !~ >= <=
  D_RAISE_STAR,
  
  //LITERALS
  L_IDENTIFIER, L_STRING, L_FLOAT,
  
  //KEYWORDS
  K_AND, K_AS, K_AWAIT, K_ALWAYS,
  K_BUILD, K_BIND,
  K_CALL,
  K_DEFINE, K_DELETE,
  K_ELSE, K_END, K_EACH,
  K_FALSE, K_FROM, // K_FINALLY,
  K_GIVE,
  K_HOLD,
  K_IF, K_IS,
  K_JOIN, 
  K_LIFT, K_LET,
  // K_MODEL, K_MAKE,
  // K_NOT,
  K_OR, 
  K_PRINT, K_PANIC,
  K_QUIT,
  K_RETURN, K_REDO, // K_READ,
  K_SELF, K_SPECIFY, K_SHARE, K_SHIFT,
  K_TRUE, K_TRY, K_TYPE, K_THEN,
  K_UNLESS, K_UNTIL, K_USE,
  K_VOID, K_VIEW, // alternative to read
  K_WHEN,
  K_XOR,
  K_YIELD,
  K_ZONE, // possible word for arena
} TokenType;

typedef struct {
  TokenType type;
  const char* start;
  int length;
  int line;
} Token;

void initScanner(const char* source);
Token scanToken();

#endif