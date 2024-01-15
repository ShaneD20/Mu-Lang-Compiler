#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "vm.h"
#include "disassemble.h"
#include "compiler.h"
#include "memory.h"
#include "object.h"
// #include "table.h"

VM vm;

// start native functions
  static Value clockNative(int count, Value* iArgs) {
    return NUMBER_VALUE((double)clock() / CLOCKS_PER_SEC);
  }
// end native functions

void freeVM() {
  freeTable(&vm.strings);
  freeTable(&vm.globals);
  freeObjects();
}
void push(Value value) {
  *vm.top_pointer = value;
  vm.top_pointer += 1;
}
Value pop() {
  vm.top_pointer += -1; // moving down is enough to show a value as no longer in use.
  return *vm.top_pointer;
}
static Value peek(int distance) {
  return vm.top_pointer[-1 - distance];
}
static void resetStack() {
  vm.top_pointer = vm.stack;
  vm.frameCount = 0;
  // vm.openUpValues = NULL;
}
static void defineNative(const char* name, NativeFn function) {
  push(OBJECT_VALUE(copyString(name, (int)strlen(name))));
  push(OBJECT_VALUE(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}
void initVM() { //TODO compare
  resetStack();
  initTable(&vm.strings);
  initTable(&vm.globals);
  defineNative("getTime", clockNative); // native fn getTime()
}
static bool isFalsey(Value value) {
  return (IS_TF(value) && !AS_TF(value));
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i += -1) {
    CallFrame* oFrame = &vm.frames[i];
    FunctionObject* oFunction = oFrame->function_;
    size_t instruction = oFrame->ip - oFunction->chunk.code_- 1;
    int line = oFunction->chunk.lines_[instruction];
    fprintf(stderr, "[line %d] in ", line);

    if (oFunction->name_pointer == NULL) {
      fprintf(stderr, "script.\n");
    } else {
      fprintf(stderr, "%s().\n", oFunction->name_pointer->runes_);
    }
  }

  resetStack();
}

static void concatenate() {
  StringObject* right = AS_STRING(pop());
  StringObject* left = AS_STRING(pop());
  int length = left->length + right->length;

  char* runes = ALLOCATE(char, length + 1);
  memcpy(runes, left->runes_, left->length);
  memcpy(runes + left->length, right->runes_, right->length);
  runes[length] = '\0';

  StringObject* result = takeString(runes, length);
  push(OBJECT_VALUE(result));
}

static bool call(FunctionObject* iFunction, int count) {
  if (count != iFunction->arity) {
    runtimeError("Expected %d arguments, but recieved %d.", iFunction->arity, count);
    return false;
  }
  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow: too many frames.");
    return false;
  }
  CallFrame* oFrame = &vm.frames[vm.frameCount++];
  oFrame->function_ = iFunction;
  oFrame->ip = iFunction->chunk.code_;
  oFrame->slots_ = vm.top_pointer - count - 1;
  return true;
}

static bool callValue(Value called, int count) {
  if (IS_OBJECT(called)) {
    switch (OBJECT_TYPE(called)) {
      case FUNCTION_TYPE : return call(AS_FUNCTION(called), count);
      case NATIVE_TYPE : {
        NativeFn native = AS_NATIVE(called);
        Value result = native(count, vm.top_pointer - count);
        vm.top_pointer -= count + 1;
        push(result);
        return true;
      }
      default: break;
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}

static InterpretResult run() {
  CallFrame* oFrame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*oFrame->ip++)
#define READ_SHORT() (oFrame->ip += 2, (uint16_t)((oFrame->ip[-2] << 8) | oFrame->ip[-1]))
#define READ_CONSTANT() (oFrame->function_->chunk.constantPool.values_[READ_BYTE()]) //lazily treats all numbers as doubles
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(valueType(a op b)); \
  } while (false)
// while (false) is to manage trailing semicolons 

  while (true) {
#ifdef DEBUG_TRACE_EXECUTION //TODO ensure proper notation
    /* Stack Tracing */
    printf(" ");
    for (Value* slot = vm.stack; slot < vm.top_pointer; slot += 1) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    /* end Stack Tracking */
    disassembleInstruction(&oFrame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
#endif
  // start of run()
    uint8_t instruction = READ_BYTE(); //TODO
    switch (instruction) {
      case OP_CONSTANT : {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_VOID : 
        push(VALUE_VOID);
        break;
      case OP_TRUE : 
        push(TF_VALUE(true));
        break;
      case OP_FALSE : 
        push(TF_VALUE(false));
        break;
      case OP_POP : 
        pop();
        break;
      case OP_GET_LOCAL : {
        uint8_t slot = READ_BYTE();
        push(oFrame->slots_[slot]);
        break;
      }
      case OP_SET_LOCAL : {
        uint8_t slot = READ_BYTE();
        oFrame->slots_[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL : {
        StringObject* name = READ_STRING();
        Value value;
        if (!tableGet(&vm.globals, name, &value)) {
          runtimeError("Undefined variable '%s'.", name->runes_);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL : {
        StringObject* name = READ_STRING();
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL : {
        StringObject* name_ = READ_STRING();
        if (tableSet(&vm.globals, name_, peek(0))) {
          deleteEntry(&vm.globals, name_);
          runtimeError("Undefined variable '%s'.", name_->runes_);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(TF_VALUE(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER: 
        BINARY_OP(TF_VALUE, >);
        break;
      case OP_LESS: 
        BINARY_OP(TF_VALUE, <);
        break;
      case OP_ADD: 
        BINARY_OP(NUMBER_VALUE, +);
        break;
      case OP_CONCATENATE: { 
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate(); //   TODO get to be polish notation
        } else {
          runtimeError("both operands must be strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VALUE, -); 
        break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VALUE, *); 
        break;
      case OP_DIVIDE: BINARY_OP(NUMBER_VALUE, /); 
        break;
      case OP_NOT: push(TF_VALUE(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(NUMBER_VALUE(-AS_NUMBER(pop())));
        break;
      case OP_PRINT : { // hand off to debug
        printValue(pop());
        printf("\n");
        break;
      }
      case OP_JUMP : {
        uint16_t offset = READ_SHORT();
        oFrame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE : { // TODO jump if true
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) {
          oFrame->ip += offset;
        }
        break;
      }
      case OP_LOOP : {
        uint16_t offset = READ_SHORT();
        oFrame->ip -= offset;
        break;
      }
      case OP_CALL : {
        int count = READ_BYTE();
        if (!callValue(peek(count), count)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        oFrame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_RETURN : { //TODO change after implementing functions
        Value result = pop();
        vm.frameCount -= 1;

        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.top_pointer = oFrame->slots_;
        push(result);
        oFrame = &vm.frames[vm.frameCount - 1];
        break;
      }
    }
  }
#undef READ_CONSTANT // undefining macros might seem solely fastidious but it helps the C preprocessor
#undef READ_BYTE
#undef BINARY_OP
#undef READ_STRING
#undef READ_SHORT
}

InterpretResult interpret(const char* iSource) {
  FunctionObject* oFunction = compile(iSource);

  if (oFunction == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  push(OBJECT_VALUE(oFunction));
  call(oFunction, 0);

  return run();
}