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
  D_NOT_EQUAL, D_EQUAL_EQUAL,       // !~ ==
  D_GREATER_EQUAL, D_LESS_EQUAL,    // >= <=
  D_EXPONENT, D_DOT, D_COLON,       // ^* .. ::
  
  //LITERALS
  L_IDENTIFIER, L_STRING, L_FLOAT,
  
  //KEYWORDS
  K_NADA, K_AND, K_OR, K_ELSE, K_IF, K_UNLESS,
  K_UNTIL, K_REPEAT, K_JOIN, K_NEW, K_SHOW,
  K_CLASS, K_DEFINE, K_AS, K_END, K_RETURN,
  K_TRUE, K_FALSE, K_SELF,

} TokenId;

typedef struct {
  TokenId type;
  const char* start;
  int length;
  int line;
} Token;

void initScanner(const char* source);
Token scanToken();

#endif