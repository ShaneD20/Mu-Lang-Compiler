#ifndef mu_compiler_h
#define mu_compiler_h

#include "object.h" // Strings compiler-include-object
#include "vm.h"
#include "scanner.h" // tokens

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

// parse-rule
typedef void (*ParseFn)(bool canAssign); // Global Variables parse-fn-type
typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool isCaptured; // Closures is-captured-field
} Local;

typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue; // for Closures

typedef enum {
  TYPE_FUNCTION,
  TYPE_INITIALIZER, // for objects initializer-type-enum
  TYPE_METHOD,      // for objects method-type-enum
  TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing; 
  ObjFunction* function;
  FunctionType type;
  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT]; // Closures upvalues-array
  int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  bool hasSuperclass;
} ClassCompiler;

ObjFunction* compile(const char* source); // Calls and Functions compile-h
void markCompilerRoots(); // Garbage Collection mark-compiler-roots-h

#endif
