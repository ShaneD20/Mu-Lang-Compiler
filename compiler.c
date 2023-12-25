#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "precedence.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panic;
} Parser;

Parser parser;
Chunk* compilingChunk;

/* Start HELPER functions 
*/
static Chunk* currentChunk() {
  return compilingChunk;
}
static void errorAt(Token* token, const char* message) {
  if (parser.panic) {
    return;
  }
  parser.panic = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == END_OF_FILE) {
    fprintf(stderr, "at end of file");
  } else if (token->type == TOKEN_ERROR) {
    //nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}
static void error(const char* message) {
  errorAt(&parser.previous, message);
}
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}
static void advance() {
  parser.previous = parser.current;
  /* 
    almost certain could move token = scanToken to above and the bottom
    and make do {} while (token.type == TokenError)
  */
  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }
    errorAtCurrent(parser.current.start);
  }
}
static void consume(TokenType current, const char* message) {
  if (parser.current.type == current) {
    advance();
    return;
  }
  errorAtCurrent(message);
}
static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t byteA, uint8_t byteB) {
  emitByte(byteA);
  emitByte(byteB);
}
static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}
// ADD to constant table
static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}
static void emitReturn() {
  emitByte(OP_RETURN);
}
static void quitCompiler() {
  emitReturn();
  #ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
      disassembleChunk(currentChunk(), "code");
    }
  #endif
}
// static void expression();

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VALUE(value));
}
void parsePrecedence(Precedence rule) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect an expression.");
    return;
  }

  bool canAssign = rule <= ASSIGNMENT_PRECEDENCE;
  prefixRule(canAssign);

  while (rule <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }
  // if (canAssign && match(S_EQUAL)) {
  //   error("Invalid assignment target.");
  // }
}
static void expression() {
  parsePrecedence(ASSIGNMENT_PRECEDENCE);
}

static void grouping() {
  expression();
  consume(S_RIGHT_PARENTHESIS, "Expext a ')' to complete expression");
}

/* End of HELPER functions 
*/
static void binary() {
  TokenType operator = parser.previous.type;
  ParseRule* rule = getRule(operator);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operator) {
    case D_BANG_TILDE: emitBytes(OP_EQUAL, OP_NOT);
      break;
    case D_EQUAL: emitByte(OP_EQUAL);
      break;
    case S_GREATER: emitByte(OP_GREATER);
      break;
    case D_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT);
      break;
    case S_LESS: emitByte(OP_LESS);
      break;
    case D_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT);
      break;
    case S_PLUS: emitByte(OP_ADD);
      break;
    case S_MINUS: emitByte(OP_SUBTRACT);
      break;
    case S_STAR: emitByte(OP_MULTIPLY);
      break;
    case S_SLASH: emitByte(OP_DIVIDE);
      break;
    default: 
      return; // unreachable
  }
}
static void unary() {
  TokenType operator = parser.previous.type;
  expression(); //compiles the operand
  parsePrecedence(UNARY_PRECEDENCE);

  switch (operator) {
    case S_MINUS: emitByte(OP_NEGATE);
      break;
    default: return; //unreachable
  }
}
static void literal() {
  switch (parser.previous.type) {
    case K_FALSE: emitByte(OP_FALSE);
      break;
    case K_VOID: emitByte(OP_VOID);
      break;
    case K_TRUE: emitByte(OP_TRUE);
      break;
    default: //unreachable
      return;
  }
}
static void string() {
  emitConstant(OBJECT_VALUE(
    copyString(parser.previous.start + 1, parser.previous.length - 2))
  );
  // TODO add escape characters
}

ParseRule rules[] = {
  [END_OF_FILE] = {NULL, NULL, ZERO_PRECEDENCE},
  [TOKEN_ERROR] = {NULL, NULL, ZERO_PRECEDENCE},
  [L_IDENTIFIER] = {NULL, NULL, ZERO_PRECEDENCE},
  [L_STRING] = {string, NULL, ZERO_PRECEDENCE},
  [L_FLOAT] = {number, NULL, ZERO_PRECEDENCE},
  [S_STAR] = {NULL, binary, FACTOR_PRECEDENCE},
  [S_SLASH] = {NULL, binary, FACTOR_PRECEDENCE},
  [S_MINUS] = {unary, binary, SUM_PRECEDENCE},
  [S_PLUS] = {NULL, binary, SUM_PRECEDENCE},
  [S_BANG] = {unary, NULL, ZERO_PRECEDENCE},
  [S_LEFT_PARENTHESIS] = {grouping, NULL, CALL_PRECEDENCE},
  [S_DOT] = {NULL, NULL, CALL_PRECEDENCE},
  [S_RIGHT_PARENTHESIS] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_LEFT_CURLYBRACE] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_RIGHT_CURLYBRACE] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_COMMA] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_SEMICOLON] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_GREATER] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_BANG_TILDE] = {NULL, binary, EQUALITY_PRECEDENCE},
  [D_EQUAL] = {NULL, binary, EQUALITY_PRECEDENCE},
  [S_LESS] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_GREATER_EQUAL] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_LESS_EQUAL] = {NULL, binary, COMPARISON_PRECEDENCE},
  [K_AND] = {NULL, NULL, AND_PRECEDENCE},
  [K_OR] = {NULL, NULL, OR_PRECEDENCE},
  [K_BUILD] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_DEFINE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_ELSE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_END] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_JOIN] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_RETURN] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_SELF] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_FALSE] = {literal, NULL, ZERO_PRECEDENCE},
  [K_TRUE] = {literal, NULL, ZERO_PRECEDENCE},
  [K_VOID] = {literal, NULL, ZERO_PRECEDENCE}, 
  [K_UNLESS] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_UNTIL] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_WHILE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_XOR] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_YIELD] = {NULL, NULL, ZERO_PRECEDENCE},
};

static ParseRule* getRule(TokenType token) {
  return &rules[token];
}

bool compile(const char* source, Chunk* chunk) {
  initScanner(source);
  compilingChunk = chunk;
  parser.hadError = false;
  parser.panic = false;
  advance();
  expression();
  consume(END_OF_FILE, "Expect complete expression");
  quitCompiler();
  return !parser.hadError;
}