#ifndef mu_parser_h
#define mu_parser_h

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
  Token head;
  Token previous;
  Token caboose;// added for type assignment let id : 6;
  Token tail;   // added for type assignment let id : 6;
  bool hasError;
  bool panicMode;
} Parser;

typedef enum {
  LVL_NONE,
  LVL_BASE,    // : :=
  LVL_OR,      // or
  LVL_AND,     // and
  LVL_EQUAL,   // == !~
  LVL_COMPARE, // < > <= >=
  LVL_SUM,     // + -
  LVL_SCALE,   // * /
  LVL_UNARY,   // ! -
  LVL_CALL,    // . ()
  LVL_PRIMARY
} Precedence;

// parse-rule
typedef void (*ParseFn)(bool canAssign); // Global Variables parse-fn-type
typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Token currentToken();
Token secondToken();
Token thirdToken();
Token fourthToken();
void setCurrent(Token token);
void parserError(bool hasError);
bool hasError();
bool panic();
void stopPanic();
void errorAt(Token* token, const char* message);
void error(const char* message);
void errorAtCurrent(const char* message);
void advance();
void shorten(); // EXPERIMENTAL
bool tokenIs(Lexeme test);
bool tokenIsNot(Lexeme test);
bool previousIsNot(Lexeme test);
bool consume(Lexeme glyph); // maybe rename
void require(Lexeme test, const char* message);
void synchronize();

#endif