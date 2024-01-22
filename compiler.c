#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h" // Garbage Collection compiler-include-memory
#include "scanner.h"
#include "parser.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

const int argLimit = 255;

Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

/* START HELPER FUNCTIONS */

static Chunk* currentChunk() {
  return &current->function->chunk;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, previousToken().line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitCompound(uint8_t operation, uint8_t byte1, uint8_t byte2, uint8_t target) {
  // TODO clean up again
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
    emitByte(OP_NIL); // I frequently wonder if false would be prefereable
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

static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;
  if (type != FT_SCRIPT) {
    current->function->name = copyString(previousToken().start, previousToken().length);
  }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
// Methods and Initializers slot-zero
  if (type != FT_FUNCTION) {
    local->name.start = "self";
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

static uint8_t identifierConstant(Token* token) {
  return makeConstant(OBJ_VAL(copyString(token->start, token->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}
/* END HELPER FUNCTIONS */

static int resolveLocal(Compiler* compiler, Token* token) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(token, &local->name)) {

      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

// Closures
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

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
  if (name->lexeme == L_VARIABLE) return -1; // prevents mutables from being an upvalue

  int depth = resolveLocal(compiler->enclosing, name);
  if (depth != -1) { // mark-local-captured
    compiler->enclosing->locals[depth].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)depth, true);
  }

// recursive
  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }
  return -1;
}

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

static void declareVariable(bool immutable) {
  if (immutable && current->scopeDepth == 0) return; 
  /* 
    bool immutable forces mutables to be stack allocated
    obtains desired behavior but may not be long term solution
  */
  Token prior = previousToken();
  Token* name = &prior;

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

static uint8_t parseVariable(const char* errorMessage) {
  if (tokenIs(L_VARIABLE)) {
    require(L_VARIABLE, errorMessage);
    declareVariable(false);
  } else {      
    require(L_IDENTIFIER, errorMessage);
    declareVariable(true);
  }
  if (current->scopeDepth > 0) {
    return 0; // Local Variables parse-local
  }
  Token prior = previousToken();
  return identifierConstant(&prior);
}

static void markInitialized() {
  if (current->scopeDepth == 0) {
    return; 
  }
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineConstant(uint8_t global) { // Currently assigns constants
  if (current->scopeDepth > 0) {
    markInitialized(); // define local
    return;
  } 
  emitBytes(OP_DEFINE_GLOBAL, global);
}

/*********************/
/* BEGIN EXPRESSIONS */
/*********************/
static void resolveExpression(Precedence precedence);
static ParseRule* getRule(Lexeme glyph); 

static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (tokenIsNot(S_RIGHT_PARENTHESES)) {
    do {
      resolveExpression(LVL_BASE);

      if (argCount >= argLimit) {
        error("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (consume(S_COMMA));
  }
  require(S_RIGHT_PARENTHESES, "Expect ')' after arguments.");
  return argCount;
}

static void andJump(bool x) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  resolveExpression(LVL_AND);
  patchJump(endJump);
}

static void orJump(bool x) {
  int endJump = emitJump(OP_JUMP_IF_TRUE);
  emitByte(OP_POP);
  resolveExpression(LVL_OR);
  patchJump(endJump);
}

// static void structure(bool canAssign) {
//   // TODO look at unary
//   if (tokenIsNot(S_RIGHT_CURLY)) {
//     do {

//     } while (consume(S_COMMA));
//   }
//   require(S_RIGHT_CURLY, "Expect '}' to finish structure literal.");
// }

// static void self_(bool unused) { // TODO delete or replace
//   if (currentClass == NULL) {
//     error("Can't use 'self' outside of a class.");
//     return;
//   }
//   variable(false);
// } 

static void binary(bool canAssign) {
  Lexeme operator = previousToken().lexeme;
  ParseRule* rule = getRule(operator);    // get the precedence
  resolveExpression((Precedence)(rule->precedence + 1)); // apply the precedence

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

static void call(bool canAssign) { 
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) { // product type
  require(L_IDENTIFIER, "Expect property name after '.' .");
  Token prior = previousToken();
  uint8_t name = identifierConstant(&prior);

  if (canAssign && consume(D_COLON_EQUAL)) { // TODO remove for immutability
    resolveExpression(LVL_BASE);
    emitBytes(OP_SET_PROPERTY, name);

  } else if (consume(S_LEFT_PARENTHESES)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);

  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

static void grouping(bool unused) {
  resolveExpression(LVL_BASE);
  require(S_RIGHT_PARENTHESES, "Expect ')' after expression.");
}

static void number(bool unused) {
  double value = strtod(previousToken().start, NULL);
  emitConstant(NUMBER_VAL(value)); // already prints
}

static void string(bool unused) {
  emitConstant(OBJ_VAL(copyString(previousToken().start + 1, previousToken().length - 2))); // already prints
}

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
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
   // printf("upvalue:"); // REMOVE
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
   // printf("global:\n"); // REMOVE
  }
  if (canAssign && consume(D_COLON_EQUAL)) {  // ugly but it works
    resolveExpression(LVL_BASE);
    emitBytes(setOp, (uint8_t)arg);
  } else if (canAssign && consume(D_PLUS_EQUAL)) {
    resolveExpression(LVL_BASE);
    emitCompound(OP_ADD, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && consume(D_STAR_EQUAL)) {
    resolveExpression(LVL_BASE);
    emitCompound(OP_MULTIPLY, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && consume(D_SLASH_EQUAL)) {
    resolveExpression(LVL_BASE);
    emitCompound(OP_DIVIDE, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && consume(D_MODULO_EQUAL)) {
    resolveExpression(LVL_BASE);
    emitCompound(OP_MODULO, getOp, setOp, (uint8_t)arg);
  } else if (canAssign && consume(D_DOT_EQUAL)) {
    resolveExpression(LVL_BASE);
    emitCompound(OP_CONCATENATE, getOp, setOp, (uint8_t)arg);
    // integer concatenation ?
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool canAssign) { // would really like to clear this confusion, but it's needed.
  namedVariable(previousToken(), canAssign); 
}

static void unary(bool unused) {
  Lexeme operator = previousToken().lexeme; // hold onto the operator, resolve the next expression, then emit

  resolveExpression(LVL_UNARY);
  switch (operator) {
    case S_BANG: emitByte(OP_NOT); 
      break;
    case S_MINUS: emitByte(OP_NEGATE); 
      break;
    default: return; // Unreachable.
  }
}

static void literal(bool canAssign) {
  
  switch (previousToken().lexeme) {
    case K_FALSE: emitByte(OP_FALSE); 
      break;
    case K_NULL:  emitByte(OP_NIL); 
      break;
    case K_TRUE:  emitByte(OP_TRUE); 
      break;
    default: return; // Unreachable.
  }
}

ParseRule rules[] = {
//                         prefix, infix, precedence
// Calls and FunctiLVL
  [S_LEFT_PARENTHESES]  = {grouping, call,   LVL_CALL},
  [S_RIGHT_PARENTHESES] = {NULL,     NULL,   LVL_NONE},
  [S_DOT]               = {NULL,     dot,    LVL_CALL},
// Sructures (Product Types, Arrays)
  // [S_LEFT_CURLY]   = {structure, NULL, LVL_NONE}, 
  [S_RIGHT_CURLY]  = {NULL, NULL, LVL_NONE},
  [S_LEFT_SQUARE]  = {NULL, NULL, LVL_NONE},
  [S_RIGHT_SQUARE] = {NULL, NULL, LVL_NONE},
// Delimiters, Guards, and Terminators
  [S_COMMA]        = {NULL, NULL, LVL_NONE},
  [D_COMMA]        = {NULL, NULL, LVL_NONE},
  [S_SEMICOLON]    = {NULL, NULL, LVL_NONE},
  [S_QUESTION]     = {NULL, NULL, LVL_NONE},
  //[NEW_LINE]        = {NULL,     NULL,   PREC_NONE},
// Assignment Operators
  [S_COLON]        = {NULL, NULL, LVL_NONE},
  [D_COLON_EQUAL]  = {NULL, NULL, LVL_NONE},
  [D_PLUS_EQUAL]   = {NULL, NULL, LVL_NONE},
  [D_MODULO_EQUAL] = {NULL, NULL, LVL_NONE},
  [D_STAR_EQUAL]   = {NULL, NULL, LVL_NONE},
  [D_SLASH_EQUAL]  = {NULL, NULL, LVL_NONE},
  [D_DOT_EQUAL]    = {NULL, NULL, LVL_NONE},
// Arithmetic, Concatenation Operators
  [K_TO]          = {NULL,     binary, LVL_SUM},
  [S_MINUS]       = {unary,    binary, LVL_SUM},
  [S_PLUS]        = {NULL,     binary, LVL_SUM},
  [S_SLASH]       = {NULL,     binary, LVL_SCALE},
  [S_MODULO]      = {NULL,     binary, LVL_SCALE},
  [S_STAR]        = {NULL,     binary, LVL_SCALE},
// Equality
  [S_BANG]          = {unary,    NULL,   LVL_UNARY},
  [D_BANG_TILDE]    = {NULL,     binary, LVL_EQUAL},
  [S_EQUAL]         = {NULL,     binary, LVL_EQUAL},
// Comparisn
  [S_GREATER]       = {NULL,     binary, LVL_COMPARE},
  [D_GREATER_EQUAL] = {NULL,     binary, LVL_COMPARE},
  [S_LESS]          = {NULL,     binary, LVL_COMPARE},
  [D_LESS_EQUAL]    = {NULL,     binary, LVL_COMPARE},
// Literals: Constants, Variables, Strings, Numbers (doubles)
  [L_IDENTIFIER]    = {variable,  NULL,   LVL_NONE},
  [L_VARIABLE]      = {variable,  NULL,   LVL_NONE},
  [L_STRING]        = {string,    NULL,   LVL_NONE},
  [L_NUMBER]        = {number,    NULL,   LVL_NONE},
//  KEYWORDS
  [TOKEN_PRINT]     = {NULL,     NULL,   LVL_NONE},
  [K_FALSE]         = {literal,  NULL,   LVL_NONE},
  [K_NULL]          = {literal,  NULL,   LVL_NONE},
  [K_TRUE]          = {literal,  NULL,   LVL_NONE},
  // [K_SELF]          = {self_,    NULL,   LVL_NONE},
  [K_OR]            = {NULL,     orJump,   LVL_OR},
  [K_AND]           = {NULL,     andJump, LVL_AND},
  [K_BUILD]         = {NULL,     NULL,   LVL_NONE},
  [K_LET]           = {NULL,     NULL,   LVL_NONE},
  [K_DEFINE]        = {NULL,     NULL,   LVL_NONE},
  [K_ELSE]          = {NULL,     NULL,   LVL_NONE},
  [K_IS]            = {NULL,     NULL,   LVL_NONE},
  [K_AS]            = {NULL,     NULL,   LVL_NONE},
  [K_IF]            = {NULL,     NULL,   LVL_NONE},
  [K_UNLESS]        = {NULL,     NULL,   LVL_NONE},
  [K_WHEN]          = {NULL,     NULL,   LVL_NONE},
  [K_RETURN]        = {NULL,     NULL,   LVL_NONE},
  [K_UNTIL]         = {NULL,     NULL,   LVL_NONE},
  [K_WHILE]         = {NULL,     NULL,   LVL_NONE},
  [K_QUIT]          = {NULL,     NULL,   LVL_NONE},
  [LANGUAGE_ERROR]  = {NULL,     NULL,   LVL_NONE},
  [END_OF_FILE]     = {NULL,     NULL,   LVL_NONE},
};
static ParseRule* getRule(Lexeme type) {
  return &rules[type];
}

static void resolvePrefix(Precedence hasPrecedence) {
  ParseFn callPrefix = getRule(previousToken().lexeme)->prefix;
  if (callPrefix == NULL) {
    error("Expect expression.");
    return;
  }
  callPrefix(hasPrecedence);
}

static void resolveInfix(Precedence hasPrecedence) {
    advance();
    ParseFn callInfix = getRule(previousToken().lexeme)->infix;
    callInfix(hasPrecedence);
}

static void resolveExpression(Precedence level) {   // TODO rename handleExpression? resolveExpression?
  advance();
  bool hasPrecedence = (level <= LVL_BASE);
  resolvePrefix(hasPrecedence);

  while (level <= getRule(parserCurrent().lexeme)->precedence) {
    resolveInfix(hasPrecedence);
  }

  if (hasPrecedence && consume(D_COLON_EQUAL)) {
    error("Invalid assignment target.");
  }
}

/**********************/
/* FINISH EXPRESSIONS */
/**********************/

static void compileTokens();

static void block() { // wrapped in begin/end scope else/while/until
  while (tokenIsNot(D_COMMA) && tokenIsNot(END_OF_FILE)) {
    compileTokens();
  }
  require(D_COMMA, "Expect ,, to complete a statement block.");  
}

static void blockTernary() { // if-else, unless-else
  while (tokenIsNot(D_COMMA) && tokenIsNot(K_ELSE) && tokenIsNot(END_OF_FILE)) {
    compileTokens();
  }
}

static void buildClosure(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope(); // no end scope

  require(S_LEFT_PARENTHESES, "Expect '(' after closure name, to start parameters.");
  if (tokenIsNot(S_RIGHT_PARENTHESES)) {
    do {
      current->function->arity++;
      if (current->function->arity > argLimit) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineConstant(constant);
    } while (consume(S_COMMA));
  }
  require(S_RIGHT_PARENTHESES, "Expect ')' after parameters.");

  require(K_AS, "Expect 'as' before function body.");
  block();

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function))); // Closures emit-closure
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void method() {
  require(L_IDENTIFIER, "Expect method name.");
  Token prior = previousToken();
  uint8_t constant = identifierConstant(&prior);
  FunctionType type = FT_METHOD;

  // initializer name
  if (previousToken().length == 5 && memcmp(previousToken().start, "build", 5) == 0) {
    type = FT_INITIALIZER;
  }
  buildClosure(type); // method body
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
  require(L_IDENTIFIER, "Expect class name.");
  Token className = previousToken();
  uint8_t nameConstant = identifierConstant(&className);
  declareVariable(true);

  emitBytes(OP_CLASS, nameConstant);
  defineConstant(nameConstant);

  ClassCompiler classCompiler;
  classCompiler.hasSuperclass = false; // removed logic for superclass, but may still be usefule for future grammars
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  namedVariable(className, false); // load-class
  require(K_AS, "Expect 'as' before class body."); 
  // class body
  while (tokenIsNot(D_COMMA) && tokenIsNot(END_OF_FILE)) {
    method();
  }
  require(D_COMMA, "Expect ',,' after class body.");

  emitByte(OP_POP); // Methods and Initializers pop-enclosing
  currentClass = currentClass->enclosing;
}

static void closureDeclaration() { // TODO could pass the name into the closure declaration
  advance();
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  buildClosure(FT_FUNCTION);
  defineConstant(global);
}

static void handleMutable() { 
  uint8_t local = parseVariable("Expect variable name."); 

  if (consume(D_COLON_EQUAL)) { // extra weight for an aesthetic
    resolveExpression(LVL_BASE);
  } else {
    emitByte(OP_NIL);
  }
  require(S_SEMICOLON, "Expect ':=' expression ';' to create a variable declaration.");
// TESTING stack allocation for mutables
  current->locals[current->localCount - 1].depth = current->scopeDepth;
  emitBytes(OP_SET_LOCAL, local);
}

static void ternaryStatement(OpCode condition) { // unless is jump if true, if jump if false
  advance();
  resolveExpression(LVL_BASE);
  require(S_QUESTION, "Expect a '?' after BOOLEAN test condition.");
  beginScope();
  int thenJump = emitJump(condition);

  emitByte(OP_POP); // then
  blockTernary();

  int elseJump = emitJump(OP_JUMP); 
  patchJump(thenJump);

  emitByte(OP_POP); // end

  if (consume(K_ELSE)) { 
    block();
  } else {
    require(D_COMMA, "Expect ,, to complete a ternary (if/unless) block, or 'else' to continue.");  
  }
  patchJump(elseJump);
  endScope();
}

static void whenStatement() { 
  advance();
  Token token = parserCurrent(); // hold the value to evaluate all expressions
  advance();
  require(S_COLON, "Expect ':' to start a when block. ('when' expression ':')");
  beginScope();

  while (tokenIs(K_IS)) {
    // parser.current = token;
    setCurrent(token);
    resolveExpression(LVL_BASE);
    require(S_QUESTION, "Expect 'is' comparator operand '?' to test condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    while (tokenIsNot(D_COMMA) && tokenIsNot(END_OF_FILE)) {
      compileTokens();
      emitByte(OP_QUIT);
    }
    require(D_COMMA, "Expect ,, to complete an 'is' block to finish a 'when' statement.");
    patchJump(thenJump); 
  }
  emitByte(OP_QUIT_END); // only statement that has this, might add to while and until
  endScope();
  require(D_COMMA, "Expect ,, to complete a when block.");
}

static void printStatement() {   // TODO remove
  advance();
  resolveExpression(LVL_BASE);
  require(S_SEMICOLON, "Expect: 'print' value ';'. With the ; to close the statement.");
  emitByte(OP_PRINT);
}

static void returnStatement() {
  advance();

  if (current->type == FT_SCRIPT) {
   // error("Can't return from top-level code."); // prevents top level return
  }
  if (current->scopeDepth < 1) {
    error("Can't return from top-level code."); // want to test to see if more desireable
  }

  if (consume(S_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == FT_INITIALIZER) {   // Methods and Initializers
      error("Can't return a value from an initializer.");
    }
    resolveExpression(LVL_BASE);
    require(S_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN); // TODO am I missing an OP_POP ?
  }
}

static void loopWithCondition(OpCode condition) {
  advance();
  int loopStart = currentChunk()->count;
  resolveExpression(LVL_BASE);
  require(S_QUESTION, "Expect '?' after condition, to begin conditional loop.");

  int exitJump = emitJump(condition); // false for while, true for until
  beginScope();
  emitByte(OP_POP);
  block();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
  endScope();
}

static void doBlock() {
  advance();
  beginScope();
  block();
  endScope();
}

static void variableDeclaration() {
  advance();
  if (tokenIs(L_VARIABLE)) {
    handleMutable();
    return;
  } 

  uint8_t global = parseVariable("Expect variable name.");

  if (consume(S_COLON)) {
    resolveExpression(LVL_BASE);
  } else {
    error("Need to initialize constants.");
  }
  require(S_SEMICOLON, "Expect ':' expression ';' to create a variable declaration.");
  defineConstant(global);
}

static void compileExpression() { // TODO write optionals for ')', '}', ']', ',,'
  resolveExpression(LVL_BASE);
  require(S_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP); // get the value
}

static void synchronize() {
  stopPanic();
  //parser.panicMode = false;
  Lexeme currentLexeme = parserCurrent().lexeme;

  while (currentLexeme != END_OF_FILE) {
    if (currentLexeme == S_SEMICOLON) {
      return;
    } 
    switch (currentLexeme) {
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
      default: ; // either way, it's in the while loop
    }
    advance();
  }
}

static void compileTokens() { 
  switch (parserCurrent().lexeme) {
    case K_DEFINE:    return closureDeclaration(); 
    case K_LET:       return variableDeclaration();
    case K_RETURN:    return returnStatement();
    case TOKEN_PRINT: return printStatement();
    case K_WHEN:      return whenStatement();
    case K_IF:        return ternaryStatement(OP_JUMP_IF_FALSE);
    case K_UNLESS:    return ternaryStatement(OP_JUMP_IF_TRUE);
    case K_WHILE:     return loopWithCondition(OP_JUMP_IF_FALSE);
    case K_UNTIL:     return loopWithCondition(OP_JUMP_IF_TRUE);
    case K_DO:        return doBlock();  
    default:          compileExpression();
  }
  if (panic()) {
    synchronize();
  }
}

ObjFunction* compile(const char* source) {
  initScanner(source);

  Compiler compiler; 
  initCompiler(&compiler, FT_SCRIPT); 

  parserError(false);
  //parser.panicMode = false;
  stopPanic();

  advance();
  while (!consume(END_OF_FILE)) {
    compileTokens();
  }

  // it all ends as one script function
  ObjFunction* function = endCompiler();
  return hasError() ? NULL : function;
}
// Garbage Collection 
void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
