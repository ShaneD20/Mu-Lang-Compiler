#ifndef mu_compiler_h
#define mu_compiler_h

#include "object.h" // Strings compiler-include-object
#include "vm.h"
#include "scanner.h"
#include "table.h"

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
  Table identifierTypes;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
} ClassCompiler;

ObjFunction* compile(const char* source); // Calls and Functions compile-h
void markCompilerRoots(); // Garbage Collection mark-compiler-roots-h

#endif

/*
  TODO add a type map with Number, Text, Truth, Void

  when user creates a 'struct', struct name is added to the type map
*/
