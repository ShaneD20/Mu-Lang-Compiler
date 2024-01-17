// Chunks of Bytecode
#ifndef mu_chunk_h
#define mu_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT, OP_NIL, OP_TRUE, OP_FALSE,
// Global Variables pop-op
  OP_POP,
// Local Variable operations
  OP_GET_LOCAL, OP_SET_LOCAL,
// Global Variables get-global-op
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL, OP_SET_GLOBAL,
// Closures operations
  OP_GET_UPVALUE, OP_SET_UPVALUE,
// Classes and Instances property-ops
  OP_GET_PROPERTY, OP_SET_PROPERTY,
// Superclasses
  OP_GET_SUPER,
// Comparison
  OP_EQUAL, OP_GREATER, OP_LESS,
//binary-ops
  OP_PLUS, OP_SUBTRACT,
  OP_MULTIPLY, OP_DIVIDE,
  OP_MODULO, 
  OP_CONCATENATE, // TODO implement
// mutation binary-ops
  OP_SUM_MUTATE,        // +=
  OP_DIFFERENCE_MUTATE, // -=
  OP_PRODUCT_MUTATE,    // *=
  OP_QUOTIENT_MUTATE,   // /=
// Unary-ops
  OP_NOT, OP_NEGATE,
// Global Variables op-print
  OP_PRINT,
// Jumping operations
  OP_JUMP, OP_LOOP,
  OP_JUMP_IF_FALSE,
  OP_JUMP_IF_TRUE,
// Calls and Functions op-call
  OP_CALL,
  OP_INVOKE,
  OP_SUPER_INVOKE,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
// Classes and Instances class-op
  OP_CLASS,
  OP_INHERIT,
  OP_METHOD
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  ValueArray constantPool; // constant pool
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif
