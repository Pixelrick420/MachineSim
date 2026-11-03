#ifndef LIB_H
#define LIB_H

#include <stdlib.h>
#include <stdarg.h>

typedef enum {
  MACHINE,
  STATEMENT,
  SYMLIST,
  SYMBOLS,
  BLK,
  TRANSITIONS,
  ACTIONS,
  ACTION,
  STRING,
  TERMINAL
} NodeType;

typedef struct ASTNode {
  NodeType type;
  char* value;
  int n;
  int line;
  struct ASTNode** children;
} ASTNode;

/* Name of the file being compiled, used as a prefix in diagnostics. */
extern const char* sourceName;

ASTNode* createASTNode(NodeType type, char* value, int n, ...);
void printAST(ASTNode* node, int depth);

/* Diagnostics. `line` may be 0 when no position is known. */
void reportError(int line, const char* fmt, ...);
void reportWarning(int line, const char* fmt, ...);
void fatalAt(int line, const char* fmt, ...);

#define lexError(...) fatalAt(yylineno, __VA_ARGS__)

#endif
