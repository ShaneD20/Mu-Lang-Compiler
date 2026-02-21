#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h" // Garbage Collection compiler-include-memory
#include "scanner.h"
#include "parser.h"
#include "table.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#define ARG_LIMIT 255
Compiler* current = NULL;

// getter
static Chunk* currentChunk() { return &current->function->chunk; }
static void beginScope() { current->scopeDepth++; }
static void emitByte(uint8_t byte) { writeChunk(currentChunk(), byte, secondToken().line); } // TODO figure out why ! = segfaults

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0
    && current->locals[current->localCount - 1].depth > current->scopeDepth) {

    if (current->locals[current->localCount - 1].isCaptured)
    { emitByte(OP_CLOSE_UPVALUE); } // Closures end-scope
    else
    { emitByte(OP_POP); }

    current->localCount--;
  }
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

static void patchJump(int offset) {
  int jump = currentChunk()->count - offset - 2; // -2 to adjust for the bytecode for the jump offset itself.

  if (jump > UINT16_MAX)
  { error("Too much code to jump over."); }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void emitReturn() {
  if (current->type == FT_INITIALIZER)
  { emitBytes(OP_GET_LOCAL, 0); }
  else
  { emitByte(OP_NIL); }

  emitByte(OP_RETURN);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}

static void emitConstant(Value value) { emitBytes(OP_CONSTANT, makeConstant(value)); }
static uint8_t identifierConstant(Token* token) { return makeConstant(OBJ_VAL(copyString(token->start, token->length))); }

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) { return false; }

  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, Token* token) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(token, &local->name)) {
      if (local->depth == -1) { error("Can't read local variable in its own initializer."); }

      return i;
    }
  }
  return -1;
}

static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;
  initTable(&current->identifierTypes); // TODO use table to static type checking

  if (type != FT_SCRIPT)
  { current->function->name = copyString(secondToken().start, secondToken().length); }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;

  if (type != FT_FUNCTION) {
    local->name.start = "self"; // TODO implement structs with methods
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
  if (!parser.hadError) { // Calls and Functions disassemble end
    disassembleChunk(currentChunk(), function->name != NULL
        ? function->name->chars : "<script>");
  }
#endif
//^ dump-chunk

  current = current->enclosing; // restore-enclosing
  return function;
}

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) { // Closures
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];

    if (upvalue->index == index && upvalue->isLocal == isLocal) { return i; }
  }

  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }
  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler* compiler, Token* name) { // Closures
  Token test = *name;
  if (test.lexeme == L_MUTABLE || compiler->enclosing == NULL)
  { return -1; }
  else {
    int depth = resolveLocal(compiler->enclosing, name);
    if (depth != -1) { // mark-local-captured
      compiler->enclosing->locals[depth].isCaptured = true;
      return addUpvalue(compiler, (uint8_t)depth, true);
    }
  // recursive
    int upvalue = resolveUpvalue(compiler->enclosing, name);

    if (upvalue != -1) { return addUpvalue(compiler, (uint8_t)upvalue, false); }
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

static void checkLocals() {
  if (current->scopeDepth == 0) { return; }

  Token prior = secondToken();
  Token* name = &prior;

  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth)
    {  break; } // does it exist in scope?
    if (identifiersEqual(name, &local->name))
    { error("Already a variable with this name in this scope."); }
  }
  addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage) {
  if (tokenIs(L_IDENTIFIER))
  { require(L_IDENTIFIER, errorMessage); }
  else
  { require(L_MUTABLE, errorMessage); }

  checkLocals();
  if (current->scopeDepth > 0)
  { return 0; } // Local Variables

  Token prior = secondToken();
  return identifierConstant(&prior);
}

static void markInitialized() {
  if (current->scopeDepth == 0)
  { return; }
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineConstant(uint8_t global) { // Currently assigns constants
  if (current->scopeDepth > 0) {
    markInitialized(); // define local
    return;
  }
  emitBytes(OP_DEFINE_GLOBAL, global);
}
/*******************END HELPER FUNCTIONS & BEGIN EXPRESSIONS*******************/

static void resolveExpression(Precedence precedence);
static ParseRule* getRule(Lexeme glyph);

static void emitCompound(uint8_t operation, uint8_t byte1, uint8_t byte2, uint8_t target) {
  advance();
  emitBytes(byte1, target);
  resolveExpression(LVL_BASE); // gathers everything to the right of operator.
  emitByte(operation);
  emitBytes(byte2, target);
}

static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (tokenIsNot(SR_ROUND)) {
    do {
      resolveExpression(LVL_BASE);
      if (argCount >= ARG_LIMIT) {
        error("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (consume(S_COMMA));
  }
  require(SR_ROUND, "Expect ')' after arguments.");
  return argCount;
}

static void andJump(bool unused) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  resolveExpression(LVL_AND);
  patchJump(endJump);
}

static void orJump(bool unused) {
  int endJump = emitJump(OP_JUMP_IF_TRUE);
  emitByte(OP_POP);
  resolveExpression(LVL_OR);
  patchJump(endJump);
}

static void structure(bool canAssign) {} // TODO implement

static void binary(bool canAssign) {
  Lexeme operator = secondToken().lexeme;
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
    case D_DOT:           emitByte(OP_CONCATENATE);
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
    case S_AMPERSAND:     emitByte(OP_BIT_AND);
      break;
    case S_PIPE:          emitByte(OP_BIT_OR);
      break;
    case S_RAISE:         emitByte(OP_BIT_XOR);
      break;
    default: return; // Unreachable.
  }
}

static void call(bool unused) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void grouping(bool unused) {
  resolveExpression(LVL_BASE);
  require(SR_ROUND, "Expect ')' after expression.");
}

static void number(bool unused) {
  //if (tokenIs(S_BANG)) { errorAtCurrent("Cannot follow a number with a '!'");}
    // prevents segfault
  double value = strtod(secondToken().start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void string(bool unused) {
  emitConstant(OBJ_VAL(copyString(secondToken().start + 1, secondToken().length - 2)));
}

static void findVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;

  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if (name.lexeme == L_IDENTIFIER && (arg = resolveUpvalue(current, &name)) != -1) { // has to be a constant to be upvalued
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else if (current->type != FT_SCRIPT && name.lexeme != L_IDENTIFIER) {
      error("Cannot access mutables from outside the function's scope.");
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign) {
    Lexeme lexeme = currentToken().lexeme;
    switch (lexeme) {
      case S_COLON : error("Please declare variables with the 'let' keyword.");

      case D_COLON_EQUAL :
        if (name.lexeme == L_IDENTIFIER) error("Only #mutables can be reassigned.");
        advance();
        resolveExpression(LVL_BASE);
        return emitBytes(setOp, (uint8_t)arg);

      case D_PLUS_EQUAL :
        if (name.lexeme == L_IDENTIFIER) error("Only #mutables can be reassigned.");
        return emitCompound(OP_ADD, getOp, setOp, (uint8_t)arg);

      case D_STAR_EQUAL :
        if (name.lexeme == L_IDENTIFIER) error("Only #mutables can be reassigned.");
        return emitCompound(OP_MULTIPLY, getOp, setOp, (uint8_t)arg);

      case D_SLASH_EQUAL :
        if (name.lexeme == L_IDENTIFIER) error("Only #mutables can be reassigned.");
        return emitCompound(OP_DIVIDE, getOp, setOp, (uint8_t)arg);

      case D_MODULO_EQUAL :
        if (name.lexeme == L_IDENTIFIER) error("Only #mutables can be reassigned.");
        return emitCompound(OP_MODULO, getOp, setOp, (uint8_t)arg);

      case D_DOT_EQUAL :
        if (name.lexeme == L_IDENTIFIER) error("Only #mutables can be reassigned.");
        return emitCompound(OP_CONCATENATE, getOp, setOp, (uint8_t)arg);
      default : break;
    }
  }
  return emitBytes(getOp, (uint8_t)arg);
}

static void variable(bool canAssign) {
  findVariable(secondToken(), canAssign);
}
/************************ EXPRESSION STATEMENTS ************************/

static void compileTokens();

static void block(Token *marker) { // TODO
  while (tokenIsNot(SR_CURLY) && tokenIsNot(END_OF_FILE)) {
    compileTokens();
  }
  if (tokenIsNot(SR_CURLY)) {
    errorAt(marker, "Need a ,, to finish code block.");
  }
  consume(SR_CURLY);
}

static void anonymous(Token *marker) {
  while (tokenIsNot(D_STAR_R_ROUND) && tokenIsNot(END_OF_FILE)) { compileTokens(); }

  if (tokenIsNot(D_STAR_R_ROUND))
  { errorAt(marker, "Need a )* to finish an anonymous code block."); }

  consume(D_STAR_R_ROUND);
}

static void blockTernary() { // if-else, unless-else
  while (tokenIsNot(SR_CURLY)
    && tokenIsNot(K_ELSE)
    && tokenIsNot(END_OF_FILE))
  { compileTokens(); }
}

static void buildAnonymous() {
  Token marker = secondToken();
  beginScope();
  anonymous(&marker);
  endScope();
}

static void buildClosure(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  require(SL_ROUND, "Need [ Type '(' paramters ')' ] to begin a function expression.");
  beginScope(); // no end scope

  if (tokenIsNot(SR_ROUND)) { // || not K_RETURN was enforcing parameters
    do {
      current->function->arity++;

      if (current->function->arity > ARG_LIMIT)
      { errorAtCurrent("Can't have more than 255 parameters."); }

      uint8_t constant = parseVariable("Expect parameter name.");
      defineConstant(constant);
    } while (consume(S_COMMA));
  }

  Token marker = currentToken();
  require(SR_ROUND, "Need a '()' to notate a function expression, declare paratemeters from function body.");
  require(SL_CURLY, "Expect {} to begin function scope");
  block(&marker);

  ObjFunction* function = endCompiler(); // could use for redo ...
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function))); // Closures emit-closure
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void buildStructure() {
  emitByte(OP_CLASS);
  require(SL_CURLY, "Expect { which will be changed later");

  require(SR_CURLY, "Expect } which will be changed later");
}

static void buildReturn(bool unused) {
  if (current->type == FT_SCRIPT || current->type == FT_INITIALIZER)
  { error("Can't return from top-level code, or within an initializer."); }

  if (currentToken().lexeme == S_SEMICOLON)
  {  emitReturn(); }
  else {
    resolveExpression(LVL_BASE);
    emitByte(OP_RETURN);
  }
}

static void printExpression() {   // TODO remove
  resolveExpression(LVL_BASE);

  if (previousIsNot(SR_CURLY)) { require(S_SEMICOLON, "Expect: 'print' value ';'. With the ; to close the statement."); }

  emitByte(OP_PRINT);
}

static void unary(bool unused) {
  Lexeme operator = secondToken().lexeme; // hold onto the operator, resolve the next expression, then emit
  resolveExpression(LVL_UNARY); // TODO part of the issue with ! = ..?

  switch (operator) {
    case S_BANG:
      emitByte(OP_NOT);
      break;
    case S_MINUS:
      emitByte(OP_NEGATE);
      break;
    case S_TILDE:
      emitByte(OP_FLIP_BITS);
      break;
    default:
      return; // Unreachable.
  }
}

static void literal(bool unused) {
  switch (secondToken().lexeme) {
    case K_FALSE:   emitByte(OP_FALSE);
      break;
    case K_NULL:    emitByte(OP_NIL);
      break;
    case K_TRUE:    emitByte(OP_TRUE);
      break;
    case K_DONE:    emitByte(OP_DONE);
        break;
    case K_FAIL:    emitByte(OP_FAIL);
        break;
    case K_QUIT:    emitByte(OP_QUIT);
      break;
    case K_USE:     buildStructure();
      break;
    case K_VOID:  // TODO assign type
        buildClosure(FT_FUNCTION);
        break;
    case K_TEXT:  // TODO assign type
        buildClosure(FT_FUNCTION);
        break;
    case K_NUMBER:  // TODO assign type
        buildClosure(FT_FUNCTION);
        break;
    case K_TRUTH:  // TODO assign type
        buildClosure(FT_FUNCTION);
        break;
    case D_STAR_L_ROUND: buildAnonymous();
      break;
    default:
      return; // Unreachable.
  }
}

ParseRule rules[] = {
//                        prefix,  infix, precedence
  [S_DOT]              = {NULL,     call, LVL_CALL}, // call struct properties?
  [SL_ROUND]           = {grouping, call, LVL_CALL}, // update call to infer function creation or call
  [SL_CURLY]           = {structure,  NULL, LVL_NONE}, // {literal, NULL, LVL_NONE},
  [SL_SQUARE]          = {NULL,     NULL, LVL_NONE}, // {literal, NULL, LVL_NONE},
//^ function calls, product type declarations
  [S_MINUS]            = {unary,    binary, LVL_SUM},
  [S_PLUS]             = {NULL,     binary, LVL_SUM},
  [S_SLASH]            = {NULL,     binary, LVL_SCALE},
  [S_MODULO]           = {NULL,     binary, LVL_SCALE},
  [S_STAR]             = {NULL,     binary, LVL_SCALE},
//^ Arithmetic Operators
  [D_DOT]              = {NULL,     binary, LVL_SUM},
  [S_AMPERSAND]        = {NULL,     binary, LVL_SUM},
  [S_PIPE]             = {NULL,     binary, LVL_SUM},
  [S_RAISE]            = {NULL,     binary, LVL_SUM},
  [S_TILDE]            = {unary,    NULL,   LVL_UNARY},
//^ Bitwise and Concatenation Operators
  [S_BANG]             = {unary,    NULL,   LVL_UNARY},
  [D_BANG_TILDE]       = {NULL,     binary, LVL_EQUAL},
  [S_EQUAL]            = {NULL,     binary, LVL_EQUAL},
  [S_GREATER]          = {NULL,     binary, LVL_COMPARE},
  [D_GREATER_EQUAL]    = {NULL,     binary, LVL_COMPARE},
  [S_LESS]             = {NULL,     binary, LVL_COMPARE},
  [D_LESS_EQUAL]       = {NULL,     binary, LVL_COMPARE},
//^ Comparison Operators
  [K_OR]               = {NULL,     orJump,  LVL_OR},
  [K_AND]              = {NULL,     andJump, LVL_AND},
  [L_IDENTIFIER]       = {variable, NULL,    LVL_NONE},
  [L_MUTABLE]          = {variable, NULL,    LVL_NONE},
  [L_STRING]           = {string,   NULL,    LVL_NONE},
  [L_NUMBER]           = {number,   NULL,    LVL_NONE},
  [K_FALSE]            = {literal,  NULL,    LVL_NONE},
  [K_FAIL]             = {literal,  NULL,    LVL_NONE},
  [K_NULL]             = {literal,  NULL,    LVL_NONE},
  [K_TRUE]             = {literal,  NULL,    LVL_NONE},
  [K_DONE]             = {literal,  NULL,    LVL_NONE},
  // [K_MAKE]             = {literal,  NULL,    LVL_NONE},
  [K_VOID]             = {literal,  NULL,    LVL_NONE},
  [K_NUMBER]           = {literal,  NULL,    LVL_NONE},
  [K_TEXT]             = {literal,  NULL,    LVL_NONE},
  [K_TRUTH]            = {literal,  NULL,    LVL_NONE},
  [K_QUIT]             = {literal,  NULL,    LVL_NONE},
  [D_STAR_L_ROUND]     = {literal,  NULL,    LVL_NONE},
  [K_RETURN]           = {buildReturn, NULL, LVL_NONE},
  [TOKEN_PRINT]        = {NULL, NULL, LVL_NONE},     // TODO remove after implementing function
//^ Expression Tokens
  [K_AS]           = {NULL, NULL, LVL_NONE},
  [K_ELSE]         = {NULL, NULL, LVL_NONE},
  [K_IS]           = {NULL, NULL, LVL_NONE},
  [K_IF]           = {NULL, NULL, LVL_NONE},
  [K_UNLESS]       = {NULL, NULL, LVL_NONE},
  [K_WHEN]         = {NULL, NULL, LVL_NONE},
  [K_UNTIL]        = {NULL, NULL, LVL_NONE},
  [K_WHILE]        = {NULL, NULL, LVL_NONE},
//^ Keywords
  [S_COLON]        = {NULL, NULL, LVL_NONE},
  [S_COMMA]        = {NULL, NULL, LVL_NONE},
  [D_COMMA]        = {NULL, NULL, LVL_NONE},
  [S_QUESTION]     = {NULL, NULL, LVL_NONE},
  [S_SEMICOLON]    = {NULL, NULL, LVL_NONE},
  [SR_ROUND]       = {NULL, NULL, LVL_NONE},
  [SR_CURLY]       = {NULL, NULL, LVL_NONE},
  [SR_SQUARE]      = {NULL, NULL, LVL_NONE},
  [END_OF_FILE]    = {NULL, NULL, LVL_NONE},
  [LANGUAGE_ERROR] = {NULL, NULL, LVL_NONE},
  //[NEW_LINE]        = {NULL,     NULL,   PREC_NONE},
//^ Delimiters, Guards, and Terminators
  [D_COLON_EQUAL]  = {NULL, NULL, LVL_NONE},
  [D_PLUS_EQUAL]   = {NULL, NULL, LVL_NONE},
  [D_STAR_EQUAL]   = {NULL, NULL, LVL_NONE},
  [D_DOT_EQUAL]    = {NULL, NULL, LVL_NONE},
  [D_MODULO_EQUAL] = {NULL, NULL, LVL_NONE},
  [D_SLASH_EQUAL]  = {NULL, NULL, LVL_NONE},
//^ Assignment Operators (handled by prefix variable)
};
static ParseRule* getRule(Lexeme type) {
  return &rules[type];
}

static void resolvePrefix(Precedence hasPrecedence) {
  ParseFn callPrefix = getRule(secondToken().lexeme)->prefix;
  if (callPrefix == NULL) {
    error("Expect expression.");
    return;
  }
  callPrefix(hasPrecedence);
}
static void resolveInfix(Precedence hasPrecedence) {
    advance();
    ParseFn callInfix = getRule(secondToken().lexeme)->infix;
    callInfix(hasPrecedence);
}

static void resolveExpression(Precedence level) {   // TODO rename handleExpression? resolveExpression?
  advance();
  bool hasPrecedence = (level <= LVL_BASE);
  resolvePrefix(hasPrecedence);

  while (level <= getRule(currentToken().lexeme)->precedence) {
    resolveInfix(hasPrecedence);
  }

  if (hasPrecedence && consume(D_COLON_EQUAL)) {
    error("Invalid assignment target.");
  }
}

/************************ FINISH EXPRESSIONS & START STATEMENTS ************************/

static void declaration() {
  advance();
  uint8_t global = parseVariable("Expect variable name.");

  if (consume(S_COLON)) {
    resolveExpression(LVL_BASE);
  } else {
    error("Need to initialize constants. ('as' identifier':' expression ';')");
  }
  if (previousIsNot(SR_CURLY)) {
    require(S_SEMICOLON, "Expect ':' expression ';' to create a variable declaration.");
  }
  defineConstant(global);
}

static void unlessStatement(OpCode condition) {
  advance();
  resolveExpression(LVL_BASE);
  Token errorMarker = currentToken();
  require(SL_CURLY, "Expect a '{' after BOOLEAN test condition.");
  beginScope();
  int thenJump = emitJump(condition);

  emitByte(OP_POP); // then
  // require(SL_CURLY, "Expect {} to begin unless scope");
  blockTernary();
  int elseJump = emitJump(OP_JUMP);
  patchJump(thenJump);

  emitByte(OP_POP); // end

  if (tokenIs(K_ELSE)) {
      errorAt(&errorMarker, "Unless block cannot branch with else. Please remove else keyword from block.");
  }

  patchJump(elseJump);
  endScope();
  require(SR_CURLY, "Expect } to end Unless scope");
}

static void ifStatement(OpCode condition) {
  advance();
  resolveExpression(LVL_BASE);
  Token errorMarker = currentToken();
  require(SL_CURLY, "Expect a '?' after BOOLEAN test condition.");
  beginScope();
  int thenJump = emitJump(condition);

  emitByte(OP_POP); // then
  // require(SL_CURLY, "Expect {} to begin unless scope");
  blockTernary();
  int elseJump = emitJump(OP_JUMP);
  patchJump(thenJump);

  emitByte(OP_POP); // end

  if (consume(K_ELSE)) {
    Token marker = secondToken();
    block(&marker);
  } else {
    if (tokenIsNot(SR_CURLY)) {
      errorAt(&errorMarker, "Need a ,, to finish if code block.");
    }
    consume(SR_CURLY);
  }
  patchJump(elseJump);
  endScope();
}

static void whenStatement() {
  advance();
  Token token = currentToken(); // hold the value to evaluate all expressions
  advance();
  require(SL_CURLY, "Expect '{' to start a when block. ('when' expression '{}')");
  beginScope();

  while (tokenIs(K_IS)) {
    setCurrent(token);
    resolveExpression(LVL_BASE);
    require(SL_CURLY, "Expect 'is' comparator operand '{' to test condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);

    while (tokenIsNot(SR_CURLY) && tokenIsNot(END_OF_FILE))
    { compileTokens(); }

    emitByte(OP_QUIT);
    require(SR_CURLY, "Expect } to complete an 'is' block to finish a 'when' statement.");
    patchJump(thenJump);
  }
  emitByte(OP_QUIT_END); // TODO only statement that has this, might add to while and until, add 'quit' keyword
  endScope();
  require(SR_CURLY, "Expect } to complete a when block.");
}

static void scopeVariable() {
  uint8_t global = parseVariable("Expect variable name.");
  require(S_COLON, "Need ':' to create a loop scoped variable.");
  resolveExpression(LVL_BASE);

  if (tokenIsNot(S_COMMA))
  { require(S_SEMICOLON, "Expect ';' to finish loop variable declarations."); }
  defineConstant(global);
}

static void loopWithCondition(OpCode condition) {
  advance();
  beginScope();

  Token token = currentToken();
  if (consume(S_COMMA)) {
    token = currentToken();
    scopeVariable();
  }

  int loopStart = currentChunk()->count;

  if (currentToken().lexeme == K_IS) {
    setCurrent(token);
  } else if (consume(S_COMMA)) {
    scopeVariable();
    loopStart = currentChunk()->count;
  } // nice to have for two pointer technique

  resolveExpression(LVL_BASE);
  require(SL_CURLY, "Expect '{' after condition, to begin conditional loop.");
  Token marker = secondToken();

  int exitJump = emitJump(condition); // false for while, true for until
  emitByte(OP_POP);
  // require(SL_CURLY, "Expect {} to enclose loop scope");
  block(&marker);
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
  emitByte(OP_QUIT_END);
  endScope();
}

/***************** END STATEMENTS *****************/

static void compileTokens() {
  switch (currentToken().lexeme) {
     case TOKEN_PRINT: advance();   // TODO replace with function
        return printExpression();
    case K_AS:      return declaration();
    case K_WHEN:    return whenStatement();
    case K_IF:      return ifStatement(OP_JUMP_IF_FALSE);
    case K_UNLESS:  return unlessStatement(OP_JUMP_IF_TRUE);
    case K_WHILE:   return loopWithCondition(OP_JUMP_IF_FALSE);
    case K_UNTIL:   return loopWithCondition(OP_JUMP_IF_TRUE);
    default:        {
      resolveExpression(LVL_BASE);

      if (secondToken().lexeme != SR_CURLY) {  // TODO maybe write optionals for ')', '}', ']'
        require(S_SEMICOLON, "Expect ';' after expression.");
      }
      emitByte(OP_POP); // get the value
    }
  }
  if (panic()) synchronize();
}

ObjFunction* compile(const char* source) {
  initScanner(source);

  Compiler compiler;
  initCompiler(&compiler, FT_SCRIPT);
  parserError(false); // set no error
  stopPanic();        // set no panic

  advance();
  while (!consume(END_OF_FILE)) {
    compileTokens();
  }

  ObjFunction* function = endCompiler(); // it all ends as one script function
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
