#include <stdio.h>
#include "disassemble.h"
#include "value.h"

void disassembleChunk(Chunk* chunk_, const char* name_) {
  printf("== %s ==\n", name_);
  for (int offset = 0; offset < chunk_->count;) {
    offset = disassembleInstruction(chunk_, offset);
  }
}

static int constantInstruction(const char* name_, Chunk* chunk_, int offset) {
  uint8_t constant = chunk_->code_[offset + 1];
  printf("%-16s %4d '", name_, constant);
  printValue(chunk_->constantPool.values_pointer[constant]);
  printf("'\n");
  return offset + 2;
}

static int invokeInstruction(const char* name_, Chunk* chunk_, int offset) {
  uint8_t constant = chunk_->code_[offset + 1];
  uint8_t argCount = chunk_->code_[offset + 2];
  printf("%-16s (%d args) %4d '", name_, argCount, constant);
  printValue(chunk_->constantPool.values_pointer[constant]);
  printf("'\n");
  return offset + 3;
}

static int simpleInstruction(const char* name_, int offset) {
  printf("%s\n", name_);
  return offset + 1;
}

static int byteInstruction(const char* name_, Chunk* chunk_, int offset) {
  uint8_t slot = chunk_->code_[offset + 1];
  printf("%-16s %4d\n", name_, slot);
  return offset + 2; // TODO debug
}

static int jumpInstruction(const char* name_, int sign, Chunk* chunk_, int offset) {
  uint16_t jump = (uint16_t)(chunk_->code_[offset + 1] << 8);
  jump |= chunk_->code_[offset + 2];
  printf("%-16s %4d -> %d\n", name_, offset, offset + 3 + sign * jump);
  return offset + 3;
}

int disassembleInstruction(Chunk* iChunk, int offset) {
  printf("%04d ", offset);

  if (offset > 0 && iChunk->lines_[offset] == iChunk->lines_[offset - 1]) {
    printf(" | ");
  } else {
    printf("%4d ", iChunk->lines_[offset]);
  }

  uint8_t instruction = iChunk->code_[offset];

  switch (instruction) {
    case OP_CONSTANT : return constantInstruction("OP_CONSTANT", iChunk, offset);
    case OP_VOID     : return simpleInstruction("OP_VOID", offset);
    case OP_TRUE     : return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE    : return simpleInstruction("OP_FALSE", offset);
    case OP_POP      : return simpleInstruction("OP_POP", offset);
    case OP_GET_LOCAL : return byteInstruction("OP_GET_LOCAL", iChunk, offset);
    case OP_SET_LOCAL : return constantInstruction("OP_SET_LOCAL", iChunk, offset);
    case OP_GET_GLOBAL : return constantInstruction("OP_GET_GLOBAL", iChunk, offset);
    case OP_DEFINE_GLOBAL :
      return constantInstruction("OP_DEFINE_GLOBAL", iChunk, offset);
    case OP_SET_GLOBAL :
      return constantInstruction("OP_SET_GLOBAL", iChunk, offset);
    case OP_GET_UPVALUE :
      return byteInstruction("OP_GET_UPVALUE", iChunk, offset);
    case OP_SET_UPVALUE :
      return byteInstruction("OP_SET_UPVALUE", iChunk, offset);
    case OP_GET_PROPERTY :
      return constantInstruction("OP_GET_PROPERTY", iChunk, offset);
    case OP_SET_PROPERTY :
      return constantInstruction("OP_SET_PROPERTY", iChunk, offset);
    case OP_GET_SUPER :
      return constantInstruction("OP_GET_SUPER", iChunk, offset);
    case OP_EQUAL :    return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER :  return simpleInstruction("OP_GREATER", offset);
    case OP_LESS :     return simpleInstruction("OP_LESS", offset);
    case OP_ADD :      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT : return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY : return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE :   return simpleInstruction("OP_DIVIDE", offset);
    case OP_NOT :      return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE :   return simpleInstruction("OP_NEGATE", offset);
    case OP_PRINT :    return simpleInstruction("OP_PRINT", offset);
    case OP_JUMP :
      return jumpInstruction("OP_JUMP", 1, iChunk, offset);
    case OP_JUMP_IF_FALSE :
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, iChunk, offset);
    case OP_LOOP :
      return jumpInstruction("OP_LOOP", -1, iChunk, offset);
    case OP_CALL:
      return byteInstruction("OP_CALL", iChunk, offset);
    case OP_INVOKE:
      return invokeInstruction("OP_INVOKE", iChunk, offset);
    case OP_SUPER_INVOKE:
      return invokeInstruction("OP_SUPER_INVOKE", iChunk, offset);
    case OP_RETURN   : return simpleInstruction("OP_RETURN", offset);
    default:
      printf("Unknown op_code %d.\n", instruction);
      return offset + 1;
  }
}