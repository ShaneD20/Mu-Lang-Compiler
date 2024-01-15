#include <stdbool.h>
#ifndef mu_compiler_h
#define mu_compiler_h

#include "scanner.h"
#include "object.h"
#include "vm.h"

typedef struct {
  Token current;
  Token previous;
  bool hasError;
  bool panic;
} Parser;

typedef enum {
  ZERO_PRECEDENCE,
  ASSIGNMENT_PRECEDENCE, // = :
  OR_PRECEDENCE,         // or
  AND_PRECEDENCE,        // and
  EQUALITY_PRECEDENCE,   // !~ ==
  COMPARISON_PRECEDENCE, // < <= > >=
  SUM_PRECEDENCE,       // + -
  FACTOR_PRECEDENCE,     // * /
  EXPONENT_PRECEDENCE,   // ^ ^*
  UNARY_PRECEDENCE,      // not - ~
  CALL_PRECEDENCE,       // . ()
  PRIMARY_PRECEDENCE,
} Precedence;

typedef void (*ParseFn)(bool assignable);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static ParseRule* getRule(TokenType token);

typedef struct {
  Token name;
  int depth;
  // bool isCaptured;
} Local;

typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;

typedef enum {
  TYPE_FUNCTION,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_SCRIPT,
} FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing_;
  FunctionObject* function_;
  FunctionType type;
  Local locals[UNIT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

// typedef struct ClassCompiler {
  // struct ClassCompiler* enclosing_;
  // bool hasSuperClass;
// } ClassCompiler;

FunctionObject* compile(const char* source_);

// TODO where will these eventuall be??
static void grouping(bool assignable);
static void unary(bool assignable);
static void binary(bool assignable);
static void number(bool assignable);

#endif