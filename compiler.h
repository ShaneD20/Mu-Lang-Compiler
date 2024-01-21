#ifndef mu_compiler_h
#define mu_compiler_h

#include "object.h" // Strings compiler-include-object
#include "vm.h"
#include "scanner.h" // tokens

typedef struct {
  Token current;
  Token previous;
  bool hasError;
  bool panicMode;
} Parser;

typedef enum {
  LVL_NONE,
  LVL_BASE,    // : :=
  LVL_OR,      // or
  LVL_AND,     // and
  LVL_EQUAL,   // == !=
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
  FT_FUNCTION,
  FT_INITIALIZER, // for objects initializer-type-enum
  FT_METHOD,      // for objects method-type-enum
  FT_SCRIPT
} FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing; 
  ObjFunction* function;
  FunctionType type;
  Local locals[UINT8_COUNT]; // Scoped values array
  int localCount;
  Upvalue upvalues[UINT8_COUNT]; // Closures upvalues array
  int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  bool hasSuperclass; // removed logic for superclass, but may still be usefule for future grammars
} ClassCompiler;

ObjFunction* compile(const char* source); // Calls and Functions compile-h
void markCompilerRoots(); // Garbage Collection mark-compiler-roots-h

#endif
