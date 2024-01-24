// Chunks of Bytecode
#ifndef mu_chunk_h
#define mu_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
// Types of Values literal-ops
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
// Global Variables pop-op
  OP_POP,
// Local Variable operations
  OP_GET_LOCAL,
  OP_SET_LOCAL,
// Mutable Variables
  OP_DEFINE_MUTABLE,
  OP_SET_MUTABLE,
  OP_GET_MUTABLE,
// Global Constants get-global-op
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
// Closures operations
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
// Types of Values comparison-ops
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
// START A Virtual Machine binary-ops
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULO,
  OP_CONCATENATE, // TODO implement
// MU_CONCATENATE,
// Types of Values unary-ops
  OP_NOT,
  OP_NEGATE,
// Global Variables op-print
  OP_PRINT,
// Jumping operations
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_JUMP_IF_TRUE,
  OP_LOOP,
  OP_QUIT,
  OP_QUIT_END,
// Calls and Functions op-call
  OP_CALL,
  OP_INVOKE,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
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
