#ifndef LIB_H
#define LIB_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
  char *value;
  int n;
  int line;
  struct ASTNode **children;
} ASTNode;

extern const char *sourceName;

ASTNode *createASTNode(NodeType type, char *value, int n, ...);
void printAST(ASTNode *node, int depth);
void reportError(int line, const char *fmt, ...);
void reportWarning(int line, const char *fmt, ...);
void fatalAt(int line, const char *fmt, ...);

#define lexError(...) fatalAt(yylineno, __VA_ARGS__)

#endif
