#ifndef mu_chunk_h
#define mu_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_VOID,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_GET_SUPER,
  OP_EQUAL,
  OP_LESS,
  OP_GREATER,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_CONCATENATE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_JUMP_IF_TRUE,
  OP_LOOP,
  OP_CALL,
  OP_INVOKE,
  OP_SUPER_INVOKE,
  OP_CLOSURE,
  OP_CLOSURE_UPVALUE,
  OP_RETURN, // return from the current function
  OP_CLASS,
  OP_INHERIT,
  OP_METHOD,
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code_pointer; // array of bytecode
  int* lines_pointer;    // array of lines
  ValueArray constants;
} Chunk;

void initChunk(Chunk* iChunk);
void freeChunk(Chunk* iChunk);
void writeChunk(Chunk* iChunk, uint8_t byte, int line);
int addConstant(Chunk* iChunk, Value value);

#endif