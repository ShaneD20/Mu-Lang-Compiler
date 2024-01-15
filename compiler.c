#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "disassemble.h"
#endif

Parser parser;
Compiler* current_ = NULL;
Chunk* iCompilingChunk = NULL; //TODO remove

static void initCompiler(Compiler* compiler_, FunctionType type) {
  compiler_->enclosing_ = current_;
  compiler_->function_ = NULL;
  compiler_->type = type;
  compiler_->localCount = 0;
  compiler_->scopeDepth = 0;
  compiler_->function_ = newFunction();
  current_ = compiler_;

  if (type != TYPE_SCRIPT) {
    current_->function_->name_pointer = copyString(parser.previous.start_, parser.previous.length);
  }

  Local* local_ = &current_->locals[current_->localCount++];
  local_->depth = 0;
  local_->name.start_ = "";
  local_->name.length = 0;
}

/* Start HELPER functions */
static Chunk* currentChunk() {
  return &current_->function_->chunk;
}

static void errorAt(Token* token_, const char* message_) {
  if (parser.panic) {
    return;
  }
  parser.panic = true;
  fprintf(stderr, "[line %d] Error", token_->line);

  if (token_->type == END_OF_FILE) {
    fprintf(stderr, "at end of file");
  } else if (token_->type == TOKEN_ERROR) {
    //nothing
  } else {
    fprintf(stderr, " at '%.*s'", token_->length, token_->start_);
  }

  fprintf(stderr, ": %s\n", message_);
  parser.hasError = true;
}

static void error(const char* message_) {
  errorAt(&parser.previous, message_);
}
static void errorAtCurrent(const char* message_) {
  errorAt(&parser.current, message_);
}
static void advance() {
  parser.previous = parser.current;

  // TODO attempt a re-write without true
  while (true) { 
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }
    errorAtCurrent(parser.current.start_);
  }
}

static void consume(TokenType token, const char* message_) {
  if (parser.current.type == token) {
    advance();
    return;
  }
  errorAtCurrent(message_);
}

static bool check(TokenType type) {
  return parser.current.type == type;
}
static bool match(TokenType type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t byteA, uint8_t byteB) {
  emitByte(byteA);
  emitByte(byteB);
}

static void emitLoop(int start) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - start + 2;
  if (offset > UINT16_MAX) {
    error("Loop context too large.");
  }
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
  emitByte(OP_VOID);
  emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("too many constants in one chunk.");
    return 0;
  }
  return (uint8_t)constant;
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }
  currentChunk()->code_[offset] = (jump >> 8) & 0xff;
  currentChunk()->code_[offset + 1] = jump & 0xff;
}

static FunctionObject* endCompiler() {
  emitReturn();
  FunctionObject* function_ = current_->function_;
   
  #ifdef DEBUG_PRINT_CODE
    if (!parser.hasError) {
      disassembleChunk(currentChunk(), function_->name_pointer != NULL
        ? function_->name_pointer->runes_pointer : "<script>");
    }
  #endif

  current_ = current_->enclosing_;
  return function_;
}

static void beginScope() {
  current_->scopeDepth++;
}
static void endScope() {
  current_->scopeDepth--;

  while (current_->localCount > 0 &&
    current_->locals[current_->localCount - 1].depth > current_->scopeDepth) {
      emitByte(OP_POP);
      current_->localCount--;
  }
}


// oddities
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType token);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token* name_) {
  return makeConstant(OBJECT_VALUE(copyString(name_->start_, name_->length)));
  // assigns global
}


static bool identifiersEqual(Token* iA, Token* iB) {
  if (iA->length != iB->length) {
    return false;
  }
  return memcmp(iA->start_, iB->start_, iA->length) == 0;
}

static int resolveLocal(Compiler* compiler_, Token* name_) {
  for (int i = compiler_->localCount - 1; i >= 0; i--) {
    Local* local_ = &compiler_->locals[i];
    if (identifiersEqual(name_, &local_->name)) {
      if (local_->depth == -1) {
        error("Cannot read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

static void addLocal(Token name) {
  if (current_->localCount == UNIT8_COUNT) {
    error("Too many local variables for this function.");
    return;
  }
  Local* iLocal = &current_->locals[current_->localCount++];
  iLocal->name = name;
  // data->depth = current->scopeDepth;
  iLocal->depth = -1;
}

static void declareVariable() {
  if (current_->scopeDepth == 0) {
    return;
  }

  Token* name_ = &parser.previous;

  for (int i = current_->localCount - 1; i >= 0; i--) {
    Local* iLocal = &current_->locals[i];
    if (iLocal->depth != -1 && iLocal->depth < current_->scopeDepth) {
      break;
    }
    if (identifiersEqual(name_, &iLocal->name)) {
      error("A local variable with this name is already within scope.");
    }
  }
  addLocal(*name_);
}

static uint8_t parseVariable(const char* message_) {
  consume(L_IDENTIFIER, message_);

  declareVariable();
  if (current_->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}

static void markInitialized() {
  if (current_->scopeDepth == 0) {
    return;
  }
  current_->locals[current_->localCount - 1].depth = current_->scopeDepth;
}

static void defineVariable(uint8_t global) {
  if (current_->scopeDepth > 0) {
    markInitialized();
    return;
  }
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
  uint8_t count = 0;
  if (!check(S_RIGHT_PARENTHESIS)) {
    do {
      expression();
      if (count >= 255) {
        error("Cannot have more that 255 arguements.");
      }
      count++;
    } while (match(S_COMMA));
  }
  consume(S_RIGHT_PARENTHESIS, "Expect ')' after arguments.");
  return count;
}

static void and_(bool assignable) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecedence(AND_PRECEDENCE);
  patchJump(endJump);
}

static void binary(bool assignable) {
  TokenType operator = parser.previous.type;
  ParseRule* rule_ = getRule(operator);
  parsePrecedence((Precedence)(rule_->precedence + 1));

  switch (operator) {
    case D_BANG_TILDE: emitBytes(OP_EQUAL, OP_NOT);
      break;
    case D_EQUAL: emitByte(OP_EQUAL);
      break;
    case S_GREATER: emitByte(OP_GREATER);
      break;
    case D_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT);
      break;
    case S_LESS: emitByte(OP_LESS);
      break;
    case D_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT);
      break;
    case S_PLUS: emitByte(OP_ADD);
      break;
    case S_MINUS: emitByte(OP_SUBTRACT);
      break;
    case S_STAR: emitByte(OP_MULTIPLY);
      break;
    case S_SLASH: emitByte(OP_DIVIDE);
      break;
    default: return; // unreachable
  }
}

static void call(bool assignable) {
  uint8_t count = argumentList();
  emitBytes(OP_CALL, count);
}

// static void dot(bool assignable) {}

static void literal(bool assignable) {
  switch (parser.previous.type) {
    case K_FALSE: emitByte(OP_FALSE);
      break;
    case K_VOID: emitByte(OP_VOID);
      break;
    case K_TRUE: emitByte(OP_TRUE);
      break;
    default: //unreachable
      return;
  }
}

static void number(bool assignable) {
  double value = strtod(parser.previous.start_, NULL);
  emitConstant(NUMBER_VALUE(value));
}

static void or_(bool assignable) { //TODO re-write with a JUMP_IF_TRUE, currently slower than 'and'
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);
  parsePrecedence(OR_PRECEDENCE);
  patchJump(endJump);
}


static void string(bool assignable) {
  emitConstant(OBJECT_VALUE(
    copyString(parser.previous.start_ + 1, parser.previous.length - 2)
  ));
  // TODO add escape characters
}

static void namedVariable(Token name, bool assignable) {
  uint8_t getOP, setOP;
  int arg = resolveLocal(current_, &name);
  if (arg != -1) {
    getOP = OP_GET_LOCAL;
    setOP = OP_SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOP = OP_GET_GLOBAL;
    setOP = OP_SET_GLOBAL;
  }

  if (assignable && match(S_COLON)) {
    expression();
    emitBytes(setOP, (uint8_t)arg);
  } else {
    emitBytes(getOP, (uint8_t)arg);
  }
}
static void variable(bool assignable) {
  namedVariable(parser.previous, assignable);
}

// static Token syntheticToke(const char* text_) {}


static void unary(bool assignable) {
  TokenType operator = parser.previous.type;
  expression(); //compiles the operand
  parsePrecedence(UNARY_PRECEDENCE);

  switch (operator) {
    case S_BANG  : emitByte(OP_NOT); 
      break;
    case S_MINUS : emitByte(OP_NEGATE);
      break;
    default: return; //unreachable
  }
}

void parsePrecedence(Precedence rule) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect an expression.");
    return;
  }

  bool canAssign = rule <= ASSIGNMENT_PRECEDENCE;
  prefixRule(canAssign);

  while (rule <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }
  if (canAssign && match(S_COLON)) {
    error("Invalid assignment target.");
  }
}

static ParseRule* getRule(TokenType token) {
  return &rules[token];
}

static void expression() {
  parsePrecedence(ASSIGNMENT_PRECEDENCE);
}

static void block() {
  while (!check(S_RIGHT_CURLYBRACE) && !check(END_OF_FILE)) {
    declaration();
  }
  consume(S_RIGHT_CURLYBRACE, "Expext a '}' to finish a block context.");
}

static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();
  //TODO revisit, refactor
  consume(S_LEFT_PARENTHESIS, "Expect '(' after function name.");
  if (!check(S_RIGHT_PARENTHESIS)) {
    do {
      current_->function_->arity++;
      if (current_->function_->arity > 255) {
        errorAtCurrent("Cannot have more that 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (match(S_COMMA));
  }

  consume(S_RIGHT_PARENTHESIS, "Expect ')' to close parameter context.");
  consume(S_LEFT_CURLYBRACE, "Expect '{' before function body.");
  block();

  FunctionObject* function_ = endCompiler();
  emitBytes(OP_CONSTANT, makeConstant(OBJECT_VALUE(function_)));
}
static void functionDeclaration() {
  uint8_t global = parseVariable("Expect a function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

static void variableDeclaration() {
  // for globals
  uint8_t GLOBAL = parseVariable("Expect variable name.");
  if (match(S_COLON)) { // replacing equals with the humble colon
    expression();
  } else {
    emitByte(OP_VOID);
  }
  consume(S_SEMICOLON, "for constants expect \": expression ;\" to complete variable declaration");

  defineVariable(GLOBAL);
}

static void expressionStatement() {
  expression();
  consume(S_SEMICOLON, "Expect ';' after expression"); //TODO doesn't get consumed..?
  emitByte(OP_POP);
}

//TODO add unlessStatement()
static void ifStatement() { //TODO revise for mu
  // consume(S_LEFT_PARENTHESIS, "expect ( after if...");
  expression();
  consume(S_QUESTION, "expect ? to complete condition");
  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  
  int elseJump = emitJump(OP_JUMP);
  patchJump(thenJump);
  emitByte(OP_POP);
  
  if (match(K_ELSE)) {
    statement();
  }
  patchJump(elseJump);
}

static void printStatement() {
  expression();
  consume(S_SEMICOLON, "Expect ';' after value for print statement.");
  emitByte(OP_PRINT); // hands off to vm.c
}

static void returnStatement() {
  if (current_->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  } // disallows top level return, might be okay to have...

  if (match(S_SEMICOLON)) {
    emitReturn();
  } else {
    expression();
    consume(S_SEMICOLON, "Expect ';' after return statement.");
    emitByte(OP_RETURN);
  }
}

static void whileStatement() { //TODO add iterator
  int loopStart = currentChunk()->count;
  expression();
  consume(S_QUESTION, "expect '?' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);
  patchJump(exitJump);
  emitByte(OP_POP);
}
static void untilStatement() { //TODO, follow through to mirror while, add generators
  int loopStart = currentChunk()->count;
  expression();
  consume(S_QUESTION, "expect '?' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_TRUE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);
  patchJump(exitJump);
  emitByte(OP_POP);
}

static void grouping(bool assignable) {
  expression();
  consume(S_RIGHT_PARENTHESIS, "Expext a ')' to complete expression");
}

static void synchronize() {
  parser.panic = false;

  // search for what ends a statement, then what begins a statement
  while (parser.current.type != END_OF_FILE) {
    if (parser.previous.type == S_SEMICOLON) {
      return;
    }
    switch (parser.current.type) {
      case K_BUILD  :
      case K_DEFINE :
      case S_OCTO   :
      case K_WHILE  :
      case K_UNTIL  :
      case K_IF     :
      case K_UNLESS :
      case K_PRINT  :
      case K_RETURN : return;
      default : // no operations
        ;
    }
    advance();
  }
}

static void declaration() {
  if (match(K_BUILD)) {
    // classDeclaration();
  } else if (match(K_DEFINE)) {
    functionDeclaration();
  } else if (match(S_OCTO)) {
    // todo figure out how to get identifier : // #identifer =
    variableDeclaration();
  } else {
    statement();
  }

  if (parser.panic) {
    synchronize();
  }
}

static void statement() {
  if (match(K_PRINT)) {
    printStatement();
  } else if (match(K_IF)) {
    ifStatement(); //TODO make expression, add unless
  } else if (match(K_WHILE)) {
    whileStatement();
  } else if (match(K_UNTIL)) {
    untilStatement();
  } else if (match(S_LEFT_CURLYBRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

// End of HELPER functions 

ParseRule rules[] = {
  [S_LEFT_PARENTHESIS] = {grouping, call, CALL_PRECEDENCE},
  [S_RIGHT_PARENTHESIS] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_LEFT_CURLYBRACE] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_RIGHT_CURLYBRACE] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_COMMA] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_DOT] = {NULL, NULL, CALL_PRECEDENCE},
  [S_MINUS] = {unary, binary, SUM_PRECEDENCE},
  [S_PLUS] = {NULL, binary, SUM_PRECEDENCE},
  [S_SEMICOLON] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_SLASH] = {NULL, binary, FACTOR_PRECEDENCE},
  [S_STAR] = {NULL, binary, FACTOR_PRECEDENCE},
  [S_BANG] = {unary, NULL, ZERO_PRECEDENCE},
  [D_BANG_TILDE] = {NULL, binary, EQUALITY_PRECEDENCE},
  [S_COLON] = {NULL, NULL, ZERO_PRECEDENCE},
  [D_EQUAL] = {NULL, binary, EQUALITY_PRECEDENCE},
  [S_GREATER] = {NULL, binary, COMPARISON_PRECEDENCE},
  [S_LESS] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_GREATER_EQUAL] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_LESS_EQUAL] = {NULL, binary, COMPARISON_PRECEDENCE},
  [L_IDENTIFIER] = {variable, NULL, ZERO_PRECEDENCE},
  [L_STRING] = {string, NULL, ZERO_PRECEDENCE},
  [L_FLOAT] = {number, NULL, ZERO_PRECEDENCE},
  [K_AND] = {NULL, and_, AND_PRECEDENCE},
  [K_OR] = {NULL, or_, OR_PRECEDENCE},
  [K_BUILD] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_DEFINE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_ELSE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_JOIN] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_PRINT] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_RETURN] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_SELF] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_FALSE] = {literal, NULL, ZERO_PRECEDENCE},
  [K_TRUE] = {literal, NULL, ZERO_PRECEDENCE},
  [K_VOID] = {literal, NULL, ZERO_PRECEDENCE}, 
  [K_UNLESS] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_UNTIL] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_WHILE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_XOR] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_YIELD] = {NULL, NULL, ZERO_PRECEDENCE},
  [END_OF_FILE] = {NULL, NULL, ZERO_PRECEDENCE},
  [TOKEN_ERROR] = {NULL, NULL, ZERO_PRECEDENCE},
};

FunctionObject* compile(const char* source_) {
  initScanner(source_);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.hasError = false;
  parser.panic = false;
  advance();

  while (!match(END_OF_FILE)) {
    declaration();
  }
  //expression();
  //consume(END_OF_FILE, "Expect complete expression");

  FunctionObject* function_ = endCompiler();
  return parser.hasError ? NULL : function_;
}