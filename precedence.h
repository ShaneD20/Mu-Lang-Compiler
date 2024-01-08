#ifndef mu_precedence_h
#include "scanner.h"
#include "compiler.h"

typedef enum {
  ZERO_PRECEDENCE,
  ASSIGNMENT_PRECEDENCE, // =
  OR_PRECEDENCE,         // or
  AND_PRECEDENCE,        // and
  EQUALITY_PRECEDENCE,   // !~ ==
  COMPARISON_PRECEDENCE, // < <= > >=
  SUM_PRECEDENCE,       // + -
  FACTOR_PRECEDENCE,     // * /
  EXPONENT_PRECEDENCE,   // ^ ^*
  UNARY_PRECEDENCE,      // not - ~
  CALL_PRECEDENCE,       // . ()
  PRIMARY_PRECEDENCE,
} Precedence;

typedef void (*ParseFn)(bool assignable);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

static ParseRule* getRule(TokenType token);

#endif