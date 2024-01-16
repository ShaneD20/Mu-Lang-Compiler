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
// Compiling Expressions include-debug

Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

static Chunk* currentChunk() {
  return &current->function->chunk;
}

// error functions
static void errorAt(Token* token, const char* message) {
// check-panic-mode, set panic mode
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) { // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}
static void error(const char* message) {
  errorAt(&parser.previous, message);
}
static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

// helper functions

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

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

// emit functions

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
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

static void emitReturn() {
  if (current->type == TYPE_INITIALIZER) {
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

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }
  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

// Calls and Functions 
static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;
  if (type != TYPE_SCRIPT) {
    current->function->name = copyString(parser.previous.start, parser.previous.length);
  }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
// Methods and Initializers slot-zero
  if (type != TYPE_FUNCTION) {
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
/* Compiling Expressions dump-chunk < Calls and Functions disassemble-end
    disassembleChunk(currentChunk(), "code");
*/
//> Calls and Functions disassemble-end
    disassembleChunk(currentChunk(), function->name != NULL
        ? function->name->chars : "<script>");
//< Calls and Functions disassemble-end
  }
#endif
//< dump-chunk

// restore-enclosing
  current = current->enclosing;
  return function;
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
//> Closures end-scope
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
//< Closures end-scope
    current->localCount--;
  }
}

// Oddities
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

// Global Variables 
static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

// Local Variables
static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

// Local Variables
static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
//> own-initializer-error
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
//< own-initializer-error
      return i;
    }
  }
  return -1;
}
//< Local Variables resolve-local
//> Closures add-upvalue
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }
//^ existing-upvalue
// too-many-upvalues...
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

// resolve-upvalue-recurse
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
static void declareVariable() {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
// existing-in-scope
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; // [negative]
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
  consume(L_IDENTIFIER, errorMessage);
  declareVariable();
  if (current->scopeDepth > 0) return 0; // Local Variables parse-local
  return identifierConstant(&parser.previous);
}

//> Local Variables mark-initialized
static void markInitialized() {
//> Calls and Functions check-depth
  if (current->scopeDepth == 0) return;
//< Calls and Functions check-depth
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}

// Global
static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
// define-local
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

// Calls and Functions argument-list
static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(S_RIGHT_PARENTHESES)) {
    do {
      expression();
//> arg-limit
      if (argCount == 255) {
        error("Can't have more than 255 arguments.");
      }
//< arg-limit
      argCount++;
    } while (match(S_COMMA));
  }
  consume(S_RIGHT_PARENTHESES, "Expect ')' after arguments.");
  return argCount;
}

//> Jumping Back and Forth and
static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecedence(PREC_AND);
  patchJump(endJump);
}

//> Jumping Back and Forth or
static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP); // testing

  patchJump(elseJump);
  // int endJump = emitJump(OP_JUMP_IF_TRUE);
  emitByte(OP_POP);
  parsePrecedence(PREC_OR);
  patchJump(endJump);
}


//> Global Variables binary
static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); 
      break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); 
      break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); 
      break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); 
      break;
    case TOKEN_LESS:          emitByte(OP_LESS); 
      break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); 
      break;
    case S_PLUS:          emitByte(OP_ADD); 
      break;
    case S_MINUS:         emitByte(OP_SUBTRACT); 
      break;
    case S_STAR:          emitByte(OP_MULTIPLY); 
      break;
    case S_SLASH:         emitByte(OP_DIVIDE); 
      break;
    default: return; // Unreachable.
  }
}

// Calls and Functions compile-call
static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

//> Classes and Instances compile-dot
static void dot(bool canAssign) {
  consume(L_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(S_COLON)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
// Methods and Initializers parse-call
  } else if (match(S_LEFT_PARENTHESES)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
//^ Methods and Initializers parse-call
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

// Global Variables
static void literal(bool canAssign) {
  switch (parser.previous.type) {
    case K_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case K_TRUE: emitByte(OP_TRUE); break;
    default: return; // Unreachable.
  }
}

// Global Variables
static void grouping(bool canAssign) {
  expression();
  consume(S_RIGHT_PARENTHESES, "Expect ')' after expression.");
}

//> Global Variables
static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

//> Global Variables string
static void string(bool canAssign) {
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

//> Global Variables named-variable-signature
static void namedVariable(Token name, bool canAssign) {
//> Local Variables named-local
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
// Closures named-variable-upvalue
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
//^ Closures named-variable-upvalue
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
// named-variable-can-assign
  if (canAssign && match(S_COLON)) {
    expression();
    emitBytes(setOp, (uint8_t)arg); // Local Variables emit-set
  } else {
    emitBytes(getOp, (uint8_t)arg); // Local Variables emit-get
  }
}

//> Global Variables variable
static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

//> Superclasses synthetic-token
static Token syntheticToken(const char* text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}

//> Superclasses super
static void super_(bool canAssign) {
// super-errors
  if (currentClass == NULL) {
    error("Can't use 'super' outside of a class.");
  } else if (!currentClass->hasSuperclass) {
    error("Can't use 'super' in a class with no superclass.");
  }
//^ super-errors

  consume(S_DOT, "Expect '.' after 'super'.");
  consume(L_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifierConstant(&parser.previous);
  
  namedVariable(syntheticToken("this"), false);

  if (match(S_LEFT_PARENTHESES)) {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

//> Methods and Initializers this
static void this_(bool canAssign) {
// this-outside-class
  if (currentClass == NULL) {
    error("Can't use 'this' outside of a class.");
    return;
  }
//^ this-outside-class
  variable(false);
} 

// Global Variables unary
static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  // Compile the operand.

  parsePrecedence(PREC_UNARY); // unary-operand

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case S_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
  }
}

ParseRule rules[] = {
//> Calls and Functions infix-left-paren
  [S_LEFT_PARENTHESES]    = {grouping, call,   PREC_CALL},
//< Calls and Functions infix-left-paren
  [S_RIGHT_PARENTHESES]   = {NULL,     NULL,   PREC_NONE},
  [S_LEFT_CURLY]    = {NULL,     NULL,   PREC_NONE}, 
  [S_RIGHT_CURLY]   = {NULL,     NULL,   PREC_NONE},
  [S_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [D_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [S_QUESTION]      = {NULL,     NULL,   PREC_NONE},
//> Classes and Instances table-dot
  [S_DOT]           = {NULL,     dot,    PREC_CALL},
//< Classes and Instances table-dot
  [S_MINUS]         = {unary,    binary, PREC_TERM},
  [S_PLUS]          = {NULL,     binary, PREC_TERM},
  [S_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [S_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [S_STAR]          = {NULL,     binary, PREC_FACTOR},
//> Types of Values table-equality
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [S_COLON]         = {NULL,     NULL,   PREC_NONE},
//> Types of Values table-comparisons
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
//> Global Variables table-identifier
  [L_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
//> Strings table-string
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
//< Strings table-string
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
//  KEYWORDS
  [K_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [K_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [K_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [K_DEFINE]        = {NULL,     NULL,   PREC_NONE},
  [K_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [K_OR]            = {NULL,     or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [K_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {super_,   NULL,   PREC_NONE},
  [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},
  [K_TRUE]          = {literal,  NULL,   PREC_NONE},
  [K_LET]           = {NULL,     NULL,   PREC_NONE},
  [K_UNTIL]         = {NULL,     NULL,   PREC_NONE},
  [K_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }
//> Global Variables prefix-rule
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

//> infix
  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
//> Global Variables infix-rule
    infixRule(canAssign);
  }
//> Global Variables invalid-assign
  if (canAssign && match(S_COLON)) {
    error("Invalid assignment target.");
  }
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
//^ expression-body
}

// Local Variables
static void block() {
  while (!check(S_RIGHT_CURLY) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(S_RIGHT_CURLY, "Expect '}' after block.");
}

// Block for Loops (until, while) and else. TODO figure out why it's failing for else
static void blockBubble() {
  while (!check(D_COMMA) && !check(TOKEN_EOF)) {
    declaration();
  }
  consume(D_COMMA, "Expect ,, to complete a while/until/else block");
}
// Block for Conditionals : if, unless
static void blockConditional() {
  while (!check(D_COMMA) && !check(K_ELSE) && !check(TOKEN_EOF)) {
    declaration();
  }
  if (check(D_COMMA)) {
    consume(D_COMMA, "Expect ,, to complete an if/unless block, or 'else' to continue");
  } else {
    consume(K_ELSE, "Expect ,, to complete an if/unless block, or 'else' to continue");
  }
}

//> Calls and Functions compile-function
static void function(FunctionType type) {
  // TODO update with -> remove need for ()
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope(); // [no-end-scope]

  consume(S_LEFT_PARENTHESES, "Expect '(' after function name.");
//> parameters
  if (!check(S_RIGHT_PARENTHESES)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (match(S_COMMA));
  }
//< parameters
  consume(S_RIGHT_PARENTHESES, "Expect ')' after parameters.");
  consume(S_LEFT_CURLY, "Expect '{' before function body.");
  block();

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function))); // Closures emit-closure
  // Closures capture-upvalues
  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

//> Methods and Initializers method
static void method() {
  consume(L_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifierConstant(&parser.previous);
  FunctionType type = TYPE_METHOD;

  //> initializer-name
  if (parser.previous.length == 4 &&
      memcmp(parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }
  function(type); // method-body
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
  consume(L_IDENTIFIER, "Expect class name.");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

//> Methods and Initializers create-class-compiler
  ClassCompiler classCompiler;
  classCompiler.hasSuperclass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

//> Superclasses compile-superclass
  if (match(TOKEN_LESS)) {
    consume(L_IDENTIFIER, "Expect superclass name.");
    variable(false);
//> inherit-self
    if (identifiersEqual(&className, &parser.previous)) {
      error("A class can't inherit from itself.");
    }
//  superclass-variable
    beginScope();
    addLocal(syntheticToken("super"));
    defineVariable(0);
//^ superclass-variable
    namedVariable(className, false);
    emitByte(OP_INHERIT);
//  set-has-superclass
    classCompiler.hasSuperclass = true;
  }

  namedVariable(className, false); // Methods and Initializers load-class
  // Methods and Initializers class-body
  consume(S_LEFT_CURLY, "Expect '{' before class body."); 
  while (!check(S_RIGHT_CURLY) && !check(TOKEN_EOF)) {
    method();
  }
  consume(S_RIGHT_CURLY, "Expect '}' after class body.");

  // Methods and Initializers pop-class
  emitByte(OP_POP);
  
  // Superclasses end-superclass-scope
  if (classCompiler.hasSuperclass) {
    endScope();
  }
  // Methods and Initializers pop-enclosing
  currentClass = currentClass->enclosing;
}

//> Calls and Functions fun-declaration
static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

//> Global Variables var-declaration
static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(S_COLON)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(S_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

//> Global Variables expression-statement
static void expressionStatement() {
  expression();
  consume(S_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}

// START FOR STATEMENT
static void forStatement() { // TODO remove or revise
  beginScope();
  consume(S_LEFT_PARENTHESES, "Expect '(' after 'for'.");

  // for-initializer
  if (match(S_SEMICOLON)) {
    // No initializer.
  } else if (match(K_LET)) {
    varDeclaration();
  } else {
    expressionStatement();
  }
  //^ for-initializer

  int loopStart = currentChunk()->count;

  // for-exit
  int exitJump = -1;
  if (!match(S_SEMICOLON)) {
    expression();
    consume(S_SEMICOLON, "Expect ';' after loop condition.");

    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  // for-increment
  if (!match(S_RIGHT_PARENTHESES)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(S_RIGHT_PARENTHESES, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }
  //^ for-increment

  statement();
  emitLoop(loopStart);

  // exit-jump
  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endScope();
}
// END FOR STATEMENT

static void ifStatement() {
  expression();
  consume(S_QUESTION, "Expect '?' after condition and before body.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
//> pop-then
  emitByte(OP_POP);
//< pop-then
  // statement(); // TEST
  beginScope();
  blockConditional();
  endScope();

  int elseJump = emitJump(OP_JUMP); // jump-over-else

  patchJump(thenJump);
  emitByte(OP_POP); // pop-end

  // compile-else
  if (match(K_ELSE)) {
    //statement(); // TODO could enforce 'else' scope
  } 
  // patch-else
  patchJump(elseJump);
}

// Global Variables print-statement
static void printStatement() {
  expression();
  consume(S_SEMICOLON, "Expect: 'print' value ';'. With the ; to close the statement.");
  emitByte(OP_PRINT);
}

// Calls and Functions return-statement
static void returnStatement() {
//> return-from-script
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

//< return-from-script
  if (match(S_SEMICOLON)) {
    emitReturn();
  } else {
// Methods and Initializers return-from-init
    if (current->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer.");
    }
//^ Methods and Initializers return-from-init
    expression();
    consume(S_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}

// Jumping Back and Forth until-statement
static void untilStatement() {
  int loopStart = currentChunk()->count; // loop-start
  expression();
  consume(S_QUESTION, "Expect '?' after condition, to begin conditional until-loop.");

  int exitJump = emitJump(OP_JUMP_IF_TRUE);
  emitByte(OP_POP);
  // statement(); // TEST was successful TODO remove.
  beginScope();
  blockBubble();
  endScope();
//> loop
  emitLoop(loopStart);
//< loop

  patchJump(exitJump);
  emitByte(OP_POP);
}

// Jumping Back and Forth while-statement
static void whileStatement() {
  int loopStart = currentChunk()->count; // loop-start
  expression();
  consume(S_QUESTION, "Expect '?' after condition, to begin conditional while-loop.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  // statement(); // TEST was successful TODO remove.
  beginScope();
  blockBubble();
  endScope();
//> loop
  emitLoop(loopStart);
//< loop

  patchJump(exitJump);
  emitByte(OP_POP);
}

// Global Variables synchronize
static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == S_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case K_DEFINE:
      case K_LET:
      case TOKEN_FOR:
      case K_IF:
      case K_UNTIL:
      case K_WHILE:
      case TOKEN_PRINT:
      case K_RETURN:
        return;

      default:
        ; // Do nothing.
    }
    advance();
  }
}

// Global Variables declaration
static void declaration() {
  if (match(TOKEN_CLASS)) {
    classDeclaration();
  } else if (match(K_DEFINE)) {
    funDeclaration();
  } else if (match(K_LET)) {
    varDeclaration();
  } else {
    statement();
  }

//> call-synchronize
  if (parser.panicMode) synchronize();
}

// Global Variables
static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(K_IF)) {
    ifStatement();
  } else if (match(K_RETURN)) {
    returnStatement();
  } else if (match(K_UNTIL)) {
    untilStatement();
  } else if (match(K_WHILE)) {
    whileStatement();
  } else if (match(S_LEFT_CURLY)) {
    beginScope(); // increment scope count;
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

ObjFunction* compile(const char* source) {
  initScanner(source);

  Compiler compiler; // Local Variables compiler
  initCompiler(&compiler, TYPE_SCRIPT); // Calls and Functions call-init-compiler

  // init-parser-error
  parser.hadError = false;
  parser.panicMode = false;

  advance();
  // Global Variables compile
  while (!match(TOKEN_EOF)) {
    declaration();
  }

  // Calls and Functions call-end-compiler
  ObjFunction* function = endCompiler();
  return parser.hadError ? NULL : function;
}
//> Garbage Collection 
void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
