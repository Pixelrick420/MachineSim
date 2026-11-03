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
  struct ASTNode** children;
} ASTNode;

ASTNode* createASTNode(NodeType type, char* value, int n, ...);
void printAST(ASTNode* node, int depth);
#endif
