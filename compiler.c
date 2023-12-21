#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panic;
} Parser;
Parser parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,// =
  PREC_OR,        // or
  PREC_AND,       // and
  PREC_EQUALITY,  // !~ ==
  PREC_COMPARISON, // < <= > >=
  PREC_TERM,      // + -
  PREC_FACTOR,    // * /
  PREC_UNARY,     // ! - ~
  PREC_CALL,      // . ()
  PREC_PRIMARY,
} Precedence;

Chunk* compilingChunk;

static Chunk* currentChunk() {
  return compilingChunk;
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

static void quitCompiler() {
  emitReturn();
}

static void grouping() {
  expression();
  consume(S_RIGHT_PARENTHESIS, "Expext a ')' to complete expression");
}

static void unary() {
  TokenType operator = parser.previous.type;
  expression(); //compiles the operand
  parsePrecedence(PREC_UNARY);

  switch (operator) {
    case S_MINUS: emitByte(OP_NEGATE);
      break;
    default: return; //unreachable
  }
}

static void number() {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(value);
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void consume(TokenType current, const char* message) {
  if (parser.current.type == current) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

static void emitReturn() {
  emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t byteA, uint8_t byteB) {
  emitByte(byteA);
  emitByte(byteB);
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
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
    fprintf(stderr, " at '&.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void advance() {
  parser.previous = parser.current;

  //TODO replace after better implementation
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