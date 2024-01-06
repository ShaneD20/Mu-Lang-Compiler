#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "precedence.h"
#include "value.h"
#include <string.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panic;
} Parser;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[UNIT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;

Parser parser;
Compiler* iCurrent = NULL;
Chunk* iCompilingChunk = NULL; //TODO remove

static void initCompiler(Compiler* compiler) {
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  iCurrent = compiler;
}

/* Start HELPER functions */
static Chunk* currentChunk() {
  return iCompilingChunk;
}
static void errorAt(Token* iToken, const char* iMessage) {
  if (parser.panic) {
    return;
  }
  parser.panic = true;
  fprintf(stderr, "[line %d] Error", iToken->line);

  if (iToken->type == END_OF_FILE) {
    fprintf(stderr, "at end of file");
  } else if (iToken->type == TOKEN_ERROR) {
    //nothing
  } else {
    fprintf(stderr, " at '%.*s'", iToken->length, iToken->start_pointer);
  }

  fprintf(stderr, ": %s\n", iMessage);
  parser.hadError = true;
}

static void error(const char* iMessage) {
  errorAt(&parser.previous, iMessage);
}
static void errorAtCurrent(const char* iMessage) {
  errorAt(&parser.current, iMessage);
}
static void advance() {
  parser.previous = parser.current;
  /* 
    almost certain could move token = scanToken to above and the bottom
    and make do {} while (token.type == TokenError)
  */
  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }
    errorAtCurrent(parser.current.start_pointer);
  }
}
static void consume(TokenType current, const char* iMessage) {
  if (parser.current.type == current) {
    advance();
    return;
  }
  errorAtCurrent(iMessage);
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
  emitByte(offset& 0xff);
}
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}
static void patchJump(int offset) {
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }
  currentChunk()->code_pointer[offset] = (jump >> 8) & 0xff;
  currentChunk()->code_pointer[offset + 1] = jump & 0xff;
}
static void emitReturn() {
  //TODO initializer
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

static void quitCompiler() {
  emitReturn();
  #ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
      disassembleChunk(currentChunk(), "code");
    }
  #endif
}

static void number(bool assignable) {
  double value = strtod(parser.previous.start_pointer, NULL);
  emitConstant(NUMBER_VALUE(value));
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
  if (canAssign && match(S_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static uint8_t identifierConstant(Token* iName) {
  return makeConstant(OBJECT_VALUE(copyString(iName->start_pointer, iName->length)));
  // assigns global
}

static bool identifiersEqual(Token* iA, Token* iB) {
  if (iA->length != iB->length) {
    return false;
  }
  return memcmp(iA->start_pointer, iB->start_pointer, iA->length) == 0;
}

static void markInitialized() {
  iCurrent->locals[iCurrent->localCount - 1].depth = iCurrent->scopeDepth;
}

static void addLocal(Token name) {
  if (iCurrent->localCount == UNIT8_COUNT) {
    error("Too many local variables for this function.");
    return;
  }
  Local* iLocal = &iCurrent->locals[iCurrent->localCount += 1];
  iLocal->name = name;
  // data->depth = current->scopeDepth;
  iLocal->depth = -1;
}

static void declareVariable() {
  if (iCurrent->scopeDepth == 0) {
    return;
  }

  Token* iName = &parser.previous;

  for (int i = iCurrent->localCount - 1; i >= 0; i -= 1) {
    Local* iLocal = &iCurrent->locals[i];
    if (iLocal->depth != -1 && iLocal->depth < iCurrent->scopeDepth) {
      break;
    }
    if (identifiersEqual(iName, &iLocal->name)) {
      error("A local variable with this name is already within scope.");
    }
  }
  addLocal(*iName);
}

static uint8_t parseVariable(const char* iMessage) {
  consume(L_IDENTIFIER, iMessage);

  declareVariable();
  if (iCurrent->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
  if (iCurrent->scopeDepth > 0) {
    markInitialized();
    return;
  }
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void expression() {
  parsePrecedence(ASSIGNMENT_PRECEDENCE);
}

static void synchronize() {
  parser.panic = false;

  // search for what ends a statement, then what begins a statement
  while (parser.current.type != END_OF_FILE) {
    if (parser.previous.type == S_SEMICOLON) {
      return;
    }
    switch (parser.current.type) {
      case K_BUILD :
      case K_DEFINE :
      case S_OCTO :
      case K_WHILE :
      case K_UNTIL :
      case K_IF :
      case K_UNLESS :
      case K_PRINT :
      case K_RETURN :
        return;
      default : // no operations
        ;
    }
    advance();
  }
}

static void beginScope() {
  iCurrent->scopeDepth += 1;
}
static void endScope() {
  iCurrent->scopeDepth -= 1;

  while (iCurrent->localCount > 0 &&
    iCurrent->locals[iCurrent->localCount - 1].depth > iCurrent->scopeDepth) {
      emitByte(OP_POP);
      iCurrent->localCount += -1;
  }
}

// oddities
static void expression();
static void statement();
static void declaration();

static void grouping(bool assignable) {
  expression();
  consume(S_RIGHT_PARENTHESIS, "Expext a ')' to complete expression");
}

static void variableDeclaration() {
  // for globals
  uint8_t global = parseVariable("Expect variable name.");
  if (match(S_COLON)) { // replacing equals with the humble colon
    expression();
  } else {
    emitByte(OP_VOID);
  }
  consume(S_SEMICOLON, "Expect ';' to end variable declaration");

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  consume(S_SEMICOLON, "Expect ';' after expression"); 
  /* when it finds the token pops and rolls back
     the value is popped and forgotten
     which is debatedly what is desirable with \n
  */
  emitByte(OP_POP);
}

static void printStatement() {
  expression();
  consume(S_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT); // hands off to vm.c
}

static void declaration() {
  if (match(K_BUILD)) {
    // classDeclaration();
  } else if (match(K_DEFINE)) {
    // todo replace soley with lambda expression
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

static void block() {
  while (!check(S_RIGHT_CURLYBRACE) && !check(END_OF_FILE)) {
    declaration();
  }
  consume(S_RIGHT_CURLYBRACE, "Expext a '}' to finish a block context.");
}

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

static void statement() {
  if (match(K_PRINT)) {
    printStatement();
  } else if (match(K_IF)) {
    ifStatement(); //TODO make expression
  } else if (match(S_LEFT_CURLYBRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i += -1) {
    Local* data = &compiler->locals[i];
    if (identifiersEqual(name, &data->name)) {
      if (data->depth == -1) {
        error("Cannot read local variable in its own initializer.");
      }
      return i;
    }
  }
  return -1;
}

/* End of HELPER functions 
*/
static void binary(bool assignable) {
  TokenType operator = parser.previous.type;
  ParseRule* iRule = getRule(operator);
  parsePrecedence((Precedence)(iRule->precedence + 1));

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
    default: 
      return; // unreachable
  }
}
static void unary(bool assignable) {
  TokenType operator = parser.previous.type;
  expression(); //compiles the operand
  parsePrecedence(UNARY_PRECEDENCE);

  switch (operator) {
    case S_MINUS: emitByte(OP_NEGATE);
      break;
    default: return; //unreachable
  }
}
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
static void string(bool assignable) {
  emitConstant(OBJECT_VALUE(
    copyString(parser.previous.start_pointer + 1, parser.previous.length - 2)
  ));
  // TODO add escape characters
}
static void namedVariable(Token name, bool assignable) {
  uint8_t getOP, setOP;
  int arg = resolveLocal(iCurrent, &name);
  if (arg != -1) {
    getOP = OP_GET_LOCAL;
    setOP = OP_SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOP = OP_GET_GLOBAL;
    setOP = OP_SET_GLOBAL;
  }

  if (assignable && match(S_EQUAL)) {
    expression();
    emitBytes(setOP, (uint8_t)arg);
  } else {
    emitBytes(getOP, (uint8_t)arg);
  }
}
static void variable(bool assignable) {
  namedVariable(parser.previous, assignable);
}

ParseRule rules[] = {
  [END_OF_FILE] = {NULL, NULL, ZERO_PRECEDENCE},
  [TOKEN_ERROR] = {NULL, NULL, ZERO_PRECEDENCE},
  [L_IDENTIFIER] = {variable, NULL, ZERO_PRECEDENCE},
  [L_STRING] = {string, NULL, ZERO_PRECEDENCE},
  [L_FLOAT] = {number, NULL, ZERO_PRECEDENCE},
  [S_STAR] = {NULL, binary, FACTOR_PRECEDENCE},
  [S_SLASH] = {NULL, binary, FACTOR_PRECEDENCE},
  [S_MINUS] = {unary, binary, SUM_PRECEDENCE},
  [S_PLUS] = {NULL, binary, SUM_PRECEDENCE},
  [S_BANG] = {unary, NULL, ZERO_PRECEDENCE},
  [S_LEFT_PARENTHESIS] = {grouping, NULL, CALL_PRECEDENCE},
  [S_DOT] = {NULL, NULL, CALL_PRECEDENCE},
  [S_RIGHT_PARENTHESIS] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_LEFT_CURLYBRACE] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_RIGHT_CURLYBRACE] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_COMMA] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_SEMICOLON] = {NULL, NULL, ZERO_PRECEDENCE},
  [S_GREATER] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_BANG_TILDE] = {NULL, binary, EQUALITY_PRECEDENCE},
  [D_EQUAL] = {NULL, binary, EQUALITY_PRECEDENCE},
  [S_LESS] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_GREATER_EQUAL] = {NULL, binary, COMPARISON_PRECEDENCE},
  [D_LESS_EQUAL] = {NULL, binary, COMPARISON_PRECEDENCE},
  [K_AND] = {NULL, NULL, AND_PRECEDENCE},
  [K_OR] = {NULL, NULL, OR_PRECEDENCE},
  [K_BUILD] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_DEFINE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_ELSE] = {NULL, NULL, ZERO_PRECEDENCE},
  [K_JOIN] = {NULL, NULL, ZERO_PRECEDENCE},
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
};

static ParseRule* getRule(TokenType token) {
  return &rules[token];
}

bool compile(const char* iSource, Chunk* iChunk) {
  initScanner(iSource);
  Compiler compiler;
  initCompiler(&compiler);
  iCompilingChunk = iChunk;
  parser.hadError = false;
  parser.panic = false;
  advance();

  while (!match(END_OF_FILE)) {
    declaration();
  }
  //expression();
  //consume(END_OF_FILE, "Expect complete expression");

  quitCompiler();
  return !parser.hadError;
}