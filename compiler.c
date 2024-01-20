#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h" // Garbage Collection compiler-include-memory
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

const int argLimit = 255;

Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

static Chunk* currentChunk() {
  return &current->function->chunk;
}

// ERROR functions
static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) {
    return;
  } 
  parser.panicMode = true;
  fprintf(stderr, "\n[line %d] Error\n", token->line);

  if (token->type == END_OF_FILE) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) { // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hasError = true;
}
static void error(const char* message) {
  errorAt(&parser.previous, message);
}
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

//> helper functions START

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;
    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  errorAtCurrent(message);
}

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool matchAdvance(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitCompound(uint8_t operation, uint8_t byte1, uint8_t byte2, uint8_t target) {
  emitByte(operation);
  emitBytes(byte1, target);
  emitBytes(byte2, target);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }
  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void emitReturn() {
  if (current->type == FT_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }
  emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) { 
      emitByte(OP_CLOSE_UPVALUE); // Closures end-scope
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

//^ helper functions END

static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;
  if (type != FT_SCRIPT) {
    current->function->name = copyString(parser.previous.start, parser.previous.length);
  }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
// Methods and Initializers slot-zero
  if (type != FT_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

static ObjFunction* endCompiler() {
  emitReturn();
  ObjFunction* function = current->function;

//> dump-chunk
#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
// Calls and Functions disassemble end
    disassembleChunk(currentChunk(), function->name != NULL
        ? function->name->chars : "<script>");
  }
#endif
//^ dump-chunk

  current = current->enclosing; // restore-enclosing
  return function;
}

// Oddities
static void declaration();
static void expression(Precedence precedence);
static ParseRule* getRule(TokenType type);

// Global Variables 
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// Local Variables
static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {

      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }
//^ Closures

  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }
  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

// Closures 
static int resolveUpvalue(Compiler* compiler, Token* name) {
  if (compiler->enclosing == NULL) return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) { // mark-local-captured
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }

// recursive
  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }
  return -1;
}

// Local Variables
static void addLocal(Token name) {
//> too many...
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}
//< Local Variables add-local
//> Local Variables declare-variable
static void declareVariable(bool immutable) {
  if (immutable && current->scopeDepth == 0) return; 
  /* 
    bool immutable forces mutables to be stack allocated
    obtains desired behavior but may not be long term solution
  */

  Token* name = &parser.previous;

  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) { // does it exist in scope?
      break;
    }
    
    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }
//^ existing-in-scope
  addLocal(*name);
}

// Global Variables
static uint8_t parseVariable(const char* errorMessage) {
  if (check(L_VARIABLE)) {
    consume(L_VARIABLE, errorMessage);
    declareVariable(false);
  } else {
    consume(L_IDENTIFIER, errorMessage);
    declareVariable(true);
  }
  if (current->scopeDepth > 0) {
    return 0; // Local Variables parse-local
  }
  return identifierConstant(&parser.previous);
}

//> Local Variables mark-initialized
static void markInitialized() {
  if (current->scopeDepth == 0) {
    return; // Calls and Functions check-depth
  }
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

// Local
static void defineMutable(uint8_t mutable) { // Testing having mutables be stack allocated
  current->locals[current->localCount - 1].depth = current->scopeDepth;
  emitBytes(OP_SET_LOCAL, mutable);
}
// Global
static void defineVariable(uint8_t global) { // Currently assigns constants
  if (current->scopeDepth > 0) {
    markInitialized();
// define local
    return;
  } 
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(S_RIGHT_PARENTHESES)) {
    do {
      expression(PREC_ASSIGNMENT);

      if (argCount >= argLimit) {
        error("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (matchAdvance(S_COMMA));
  }
  consume(S_RIGHT_PARENTHESES, "Expect ')' after arguments.");
  return argCount;
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  expression(PREC_AND);
  patchJump(endJump);
}

static void or_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_TRUE);
  emitByte(OP_POP);
  expression(PREC_OR);
  patchJump(endJump);
}

static void binary(bool canAssign) {
  TokenType operator = parser.previous.type;
  ParseRule* rule = getRule(operator);
  expression((Precedence)(rule->precedence + 1));
  //printf("OP_binary, "); // REMOVE
  switch (operator) {
    case D_BANG_TILDE:    emitBytes(OP_EQUAL, OP_NOT); 
      break;
    case S_EQUAL:         emitByte(OP_EQUAL); 
      break;
    case S_GREATER:       emitByte(OP_GREATER); 
      break;
    case D_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); 
      break;
    case S_LESS:          emitByte(OP_LESS); 
      break;
    case D_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT);
      break;
    case K_TO:            emitByte(OP_CONCATENATE);
      break;
    case S_PLUS:          emitByte(OP_ADD); 
      break;
    case S_MINUS:         emitByte(OP_SUBTRACT); 
      break;
    case S_STAR:          emitByte(OP_MULTIPLY);
      break;
    case S_SLASH:         emitByte(OP_DIVIDE); 
      break;
    case S_MODULO:        emitByte(OP_MODULO);
      break;
    default: return; // Unreachable.
  }
}

// invoked by '('
static void call(bool canAssign) { 
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

// invoked by 'object.'
static void dot(bool canAssign) {
  consume(L_IDENTIFIER, "Expect property name after '.' .");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && matchAdvance(D_COLON_EQUAL)) { // TODO remove for immutability
    expression(PREC_ASSIGNMENT);
    emitBytes(OP_SET_PROPERTY, name);
// Methods and Initializers parse-call
  } else if (matchAdvance(S_LEFT_PARENTHESES)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
//^ Methods and Initializers parse-call
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}
//^ currently not used anywhere

// Global Variables
static void literal(bool canAssign) {
  switch (parser.previous.type) {
    case K_FALSE: emitByte(OP_FALSE); 
      break;
    case K_NULL:  emitByte(OP_NIL); 
      break;
    case K_TRUE:  emitByte(OP_TRUE); 
      break;
    default: return; // Unreachable.
  }
}

// Global Variables
static void grouping(bool canAssign) {
  expression(PREC_ASSIGNMENT);
  consume(S_RIGHT_PARENTHESES, "Expect ')' after expression.");
}

//> Global Variables
static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value)); // already prints
}

//> Global Variables string
static void string(bool canAssign) {
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2))); // already prints
}

//> Global Variables named-variable-signature
static void namedVariable(Token name, bool canAssign) { 
  uint8_t getOp, setOp;
  /*
    if mutables are stored on the stack, how can I get resolveLocal to return 0 ?
  */

  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
   // printf("local:\n"); // REMOVE
  // Local Variables
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
   // printf("upvalue:"); // REMOVE
  // Closures
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
   // printf("global:\n"); // REMOVE
  // global
  }
  if (canAssign && matchAdvance(D_COLON_EQUAL)) {
    expression(PREC_ASSIGNMENT);
    emitBytes(setOp, (uint8_t)arg);
  } else if (canAssign && matchAdvance(D_PLUS_EQUAL)) {  // - * / % .
    expression(PREC_ASSIGNMENT);
    emitCompound(OP_ADD, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && matchAdvance(D_STAR_EQUAL)) {
    expression(PREC_ASSIGNMENT);
    emitCompound(OP_MULTIPLY, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && matchAdvance(D_SLASH_EQUAL)) {
    expression(PREC_ASSIGNMENT);
    emitCompound(OP_DIVIDE, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && matchAdvance(D_MODULO_EQUAL)) {
    expression(PREC_ASSIGNMENT);
    emitCompound(OP_MODULO, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && matchAdvance(D_DOT_EQUAL)) {
    expression(PREC_ASSIGNMENT);
    emitCompound(OP_CONCATENATE, getOp, setOp, (uint8_t)arg);
    // integer concatenation ?
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

// Global Variables variable
static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign); 
}

static void structure(bool canAssign) {
  /*
    need to be like _t argumentList, create an amount of element
    then be like a function call that perpetually sits on the table
  */

  if (!check(S_RIGHT_CURLY)) {
    do {
      current->function->arity++; // TODO replace with an anonymous instance
      if (current->function->arity > argLimit) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (matchAdvance(S_COMMA));
  }
//^ parameters
  consume(S_RIGHT_CURLY, "Expect '}' to finish structure literal.");
}

static void self_(bool canAssign) { // classes
  if (currentClass == NULL) {
    error("Can't use 'self' outside of a class.");
    return;
  }
  variable(false);
} 

// Global Variables unary
static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  // Compile the operand.

  expression(PREC_UNARY); // unary-operand

  // Emit the operator instruction.
  switch (operatorType) {
    case S_BANG: emitByte(OP_NOT); 
      break;
    case S_MINUS: emitByte(OP_NEGATE); 
      break;
    default: return; // Unreachable.
  }
}

ParseRule rules[] = {
// Calls and Functions 
  [S_LEFT_PARENTHESES]  = {grouping, call,   PREC_CALL},
  [S_RIGHT_PARENTHESES] = {NULL,     NULL,   PREC_NONE},
  [S_DOT]               = {NULL,     dot,    PREC_CALL},
// Sructures
  [S_LEFT_CURLY]   = {structure, NULL, PREC_NONE}, 
  [S_RIGHT_CURLY]  = {NULL, NULL, PREC_NONE},
  [S_LEFT_SQUARE]  = {NULL, NULL, PREC_NONE},
  [S_RIGHT_SQUARE] = {NULL, NULL, PREC_NONE},
// Delimiters, Guards, and Terminators
  [S_COMMA]        = {NULL, NULL, PREC_NONE},
  [D_COMMA]        = {NULL, NULL, PREC_NONE},
  [S_SEMICOLON]    = {NULL, NULL, PREC_NONE},
  [K_END]          = {NULL, NULL, PREC_NONE},
  [S_QUESTION]     = {NULL, NULL, PREC_NONE},
  //[NEW_LINE]        = {NULL,     NULL,   PREC_NONE},
// Assignment Operators
  [S_COLON]        = {NULL, NULL, PREC_NONE},
  [D_COLON_EQUAL]  = {NULL, NULL, PREC_NONE},
  [D_PLUS_EQUAL]   = {NULL, NULL, PREC_NONE},
  [D_MODULO_EQUAL] = {NULL, NULL, PREC_NONE},
  [D_STAR_EQUAL]   = {NULL, NULL, PREC_NONE},
  [D_SLASH_EQUAL]  = {NULL, NULL, PREC_NONE},
  [D_DOT_EQUAL]    = {NULL, NULL, PREC_NONE},
// Concatenation Operator
  [K_TO]          = {NULL,     binary, PREC_TERM},
// Arithmetic Operators
  [S_MINUS]       = {unary,    binary, PREC_TERM},
  [S_PLUS]        = {NULL,     binary, PREC_TERM},
  [S_SLASH]       = {NULL,     binary, PREC_FACTOR},
  [S_MODULO]      = {NULL,     binary, PREC_FACTOR},
  [S_STAR]        = {NULL,     binary, PREC_FACTOR},
// Equality
  [S_BANG]          = {unary,    NULL,   PREC_NONE},
  [D_BANG_TILDE]    = {NULL,     binary, PREC_EQUALITY},
  [S_EQUAL]         = {NULL,     binary, PREC_EQUALITY},
// Comparisons
  [S_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [D_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [S_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [D_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
// Literals: Constants, Variables, Strings, Numbers (doubles)
  [L_IDENTIFIER]    = {variable,  NULL,   PREC_NONE},
  [L_VARIABLE]      = {variable,  NULL,   PREC_NONE},
  [L_STRING]        = {string,    NULL,   PREC_NONE},
  [L_NUMBER]        = {number,    NULL,   PREC_NONE},
//  KEYWORDS
  [TOKEN_PRINT]        = {NULL,     NULL,   PREC_NONE},
  [K_FALSE]         = {literal,  NULL,   PREC_NONE},
  [K_NULL]          = {literal,  NULL,   PREC_NONE},
  [K_TRUE]          = {literal,  NULL,   PREC_NONE},
  [K_SELF]          = {self_,    NULL,   PREC_NONE},
  [K_OR]            = {NULL,     or_,    PREC_OR},
  [K_AND]           = {NULL,     and_,   PREC_AND},
  [K_BUILD]          = {NULL,     NULL,   PREC_NONE},
  [K_LET]           = {NULL,     NULL,   PREC_NONE},
  [K_DEFINE]        = {NULL,     NULL,   PREC_NONE},
  [K_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [K_IS]            = {NULL,     NULL,   PREC_NONE},
  [K_AS]            = {NULL,     NULL,   PREC_NONE},
  [K_IF]            = {NULL,     NULL,   PREC_NONE},
  [K_UNLESS]        = {NULL,     NULL,   PREC_NONE},
  [K_WHEN]          = {NULL,     NULL,   PREC_NONE},
  [K_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [K_UNTIL]         = {NULL,     NULL,   PREC_NONE},
  [K_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [K_QUIT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]     = {NULL,     NULL,   PREC_NONE},
  [END_OF_FILE]     = {NULL,     NULL,   PREC_NONE},
};

static void expression(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

// infix
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && matchAdvance(D_COLON_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

// Local Variables
static void block() { // wrapped in begin/end scope else/while/until
  while (!check(D_COMMA) && !check(END_OF_FILE)) {
    declaration();
  }
  consume(D_COMMA, "Expect ,, to complete a statement block.");
}

static void blockTernary() { // if-else, unless-else
  while (!check(D_COMMA) && !check(K_ELSE) && !check(END_OF_FILE)) {
    declaration();
  }
  if (!check(K_ELSE)) {
    consume(D_COMMA, "Expect ,, to complete a ternary (if/unless) block, or 'else' to continue.");
  }
}

//> Calls and Functions compile-function
static void buildClosure(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope(); // no end scope

  consume(S_LEFT_PARENTHESES, "Expect '(' after closure name, to start parameters.");
  if (!check(S_RIGHT_PARENTHESES)) {
    do {
      current->function->arity++;
      if (current->function->arity > argLimit) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (matchAdvance(S_COMMA));
  }
  consume(S_RIGHT_PARENTHESES, "Expect ')' after parameters.");

  consume(K_AS, "Expect 'as' before function body.");
  block();

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function))); // Closures emit-closure
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void method() {
  consume(L_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifierConstant(&parser.previous);
  FunctionType type = FT_METHOD;

  // initializer name
  if (parser.previous.length == 5 && memcmp(parser.previous.start, "build", 5) == 0) {
    type = FT_INITIALIZER;
  }
  buildClosure(type); // method body
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
  consume(L_IDENTIFIER, "Expect class name.");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable(true);

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

  ClassCompiler classCompiler;
  classCompiler.hasSuperclass = false; // removed logic for superclass, but may still be usefule for future grammars
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  namedVariable(className, false); // load-class
  consume(K_AS, "Expect 'as' before class body."); 
  // class body
  while (!check(D_COMMA) && !check(END_OF_FILE)) {
    method();
  }
  consume(D_COMMA, "Expect ',,' after class body.");

  emitByte(OP_POP); 

  // Methods and Initializers pop-enclosing
  currentClass = currentClass->enclosing;
}

//> Calls and Functions fun-declaration
static void closureDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  buildClosure(FT_FUNCTION);
  defineVariable(global);
}

//> Global Variable declaration (testing stack allocation)
static void varDeclaration() {
  uint8_t local = parseVariable("Expect variable name."); // TESTING stack allocation for mutables

  if (matchAdvance(D_COLON_EQUAL)) {
    expression(PREC_ASSIGNMENT);
  } else {
    emitByte(OP_NIL);
  }
  consume(S_SEMICOLON, "Expect ':=' expression ';' to create a variable declaration.");

  defineMutable(local); // TODO should be local
}
//> Global Constant declaration
static void constDeclaration() { // TODO get constants to be immutable;
  uint8_t global = parseVariable("Expect variable name.");

  if (matchAdvance(S_COLON)) {
    expression(PREC_ASSIGNMENT);
  } else {
    emitByte(OP_NIL);
  }
  consume(S_SEMICOLON, "Expect ':=' expression ';' to create a variable declaration.");

  defineVariable(global);
}

//> Global Variables expression-statement
static void expressionStatement() {
  expression(PREC_ASSIGNMENT);
  // TODO write optionals for ')', '}', ']', ',,'
  consume(S_SEMICOLON, "Expect ';' after expression."); 
  emitByte(OP_POP);
}

static void ifStatement() {
  expression(PREC_ASSIGNMENT);
  consume(S_QUESTION, "Expect 'if' BOOLEAN '?' to test condition.");
  beginScope();
  int thenJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP); // then
  blockTernary(); // statement();

  int elseJump = emitJump(OP_JUMP); 
  patchJump(thenJump);

  emitByte(OP_POP); // pop-end

  if (matchAdvance(K_ELSE)) { 
    block();
  } 
  patchJump(elseJump); // set else jump at end
  endScope();
}

static void unlessStatement() {
  expression(PREC_ASSIGNMENT);
  consume(S_QUESTION, "Expect 'unless' BOOLEAN '?' to test condition.");
  beginScope();
  int thenJump = emitJump(OP_JUMP_IF_TRUE);

  emitByte(OP_POP); //then
  blockTernary();

  int elseJump = emitJump(OP_JUMP); // jump-over-else
  patchJump(thenJump);

  emitByte(OP_POP); // pop-end

  if (matchAdvance(K_ELSE)) { 
    block();
  } 
  patchJump(elseJump);
  endScope();
}

static void whenStatement() { 
  Token token = parser.current; // hold the value to evaluate all expressions
  advance();
  consume(S_COLON, "Expect ':' to start a when block. ('when' expression ':')");
  beginScope();

  while (check(K_IS)) {
    parser.current = token;
    expression(PREC_ASSIGNMENT);
    consume(S_QUESTION, "Expect 'is' comparator operand '?' to test condition.");
    int thenJump = emitJump(OP_JUMP_IF_FALSE); // skips over two instructions, jump and the next instruction.

    while (!check(D_COMMA) && !check(END_OF_FILE)) {
      declaration();
      emitByte(OP_QUIT);
    }
    consume(D_COMMA, "Expect ,, to complete an 'is' block to finish a 'when' statement.");
    patchJump(thenJump); 
  }
  emitByte(OP_QUIT_END);
  endScope();
  consume(D_COMMA, "Expect ,, to complete a when block.");
}

// Global Variables print-statement
static void printStatement() {
  expression(PREC_ASSIGNMENT);
  consume(S_SEMICOLON, "Expect: 'print' value ';'. With the ; to close the statement.");

  emitByte(OP_PRINT);
}

static void returnStatement() {
  if (current->type == FT_SCRIPT) {
    error("Can't return from top-level code."); // prevents top level return
  }

  if (matchAdvance(S_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == FT_INITIALIZER) {   // Methods and Initializers
      error("Can't return a value from an initializer.");
    }
    expression(PREC_ASSIGNMENT);
    consume(S_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}

static void untilStatement() {
  int loopStart = currentChunk()->count;
  expression(PREC_ASSIGNMENT);
  consume(S_QUESTION, "Expect '?' after condition, to begin conditional until-loop.");

  int exitJump = emitJump(OP_JUMP_IF_TRUE);
  beginScope();
  emitByte(OP_POP);
  block();
//> loop
  emitLoop(loopStart);
//^ loop
  patchJump(exitJump);
  emitByte(OP_POP);
  endScope();
}

static void whileStatement() {
  int loopStart = currentChunk()->count;
  expression(PREC_ASSIGNMENT);
  consume(S_QUESTION, "Expect '?' after condition, to begin conditional while-loop.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  beginScope();
  emitByte(OP_POP);
  block();
//> loop
  emitLoop(loopStart);
//^ loop
  patchJump(exitJump);
  emitByte(OP_POP);
  endScope();
}

// Global Variables synchronize
static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != END_OF_FILE) {
    if (parser.previous.type == S_SEMICOLON) return;
    switch (parser.current.type) {
      case K_BUILD:
      case K_DEFINE:
      case K_LET:
      case K_IF:
      case K_UNLESS:
      case K_WHEN:
      case K_UNTIL:
      case K_WHILE:
      case K_QUIT:
      case TOKEN_PRINT:
      case K_RETURN:
        return;

      default:
        ; // Do nothing.
    }
    advance();
  }
}

static void statement() {
  if (matchAdvance(TOKEN_PRINT)) {
    printStatement();
  } else if (matchAdvance(K_IF)) {
    ifStatement();
  } else if (matchAdvance(K_WHEN)) {
    whenStatement();
  } else if (matchAdvance(K_UNLESS)) {
    unlessStatement();
  } else if (matchAdvance(K_RETURN)) {
    returnStatement();
  } else if (matchAdvance(K_UNTIL)) {
    untilStatement();
  } else if (matchAdvance(K_WHILE)) {
    whileStatement();
  } else if (matchAdvance(K_DO)) {
    beginScope();
    block();
    endScope();
  } else if (matchAdvance(K_QUIT)) {
    printf("quitting, ");
    consume(S_SEMICOLON, "Expect semicolon to finish 'quit' statement");
    emitByte(OP_QUIT);
  } else {
    expressionStatement();
  }
}

// Global Variables declaration
static void declaration() { // TODO hub for assigment
  if (matchAdvance(K_DEFINE)) {
    closureDeclaration();
  } else if (matchAdvance(K_BUILD)) {
    classDeclaration();
  } else if (matchAdvance(K_LET)) {
    if (check(L_VARIABLE)) {
      varDeclaration();
    } else {
      constDeclaration();
    }
  } else {
    statement();
  }

  if (parser.panicMode) {
    synchronize();
  }
}

// Global Variables

ObjFunction* compile(const char* source) {
  initScanner(source);

  Compiler compiler; // Local Variables compiler
  initCompiler(&compiler, FT_SCRIPT); // Calls and Functions call-init-compiler

  // init-parser-error
  parser.hasError = false;
  parser.panicMode = false;

  advance();
  // Global Variables compile
  while (!matchAdvance(END_OF_FILE)) {
    declaration();
  }

  // Calls and Functions call-end-compiler
  ObjFunction* function = endCompiler();
  return parser.hasError ? NULL : function;
}
//> Garbage Collection 
void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
