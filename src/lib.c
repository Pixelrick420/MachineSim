#include <stdio.h>
#include "../include/lib.h"

ASTNode* createASTNode(NodeType type, char* value, int n, ...){
  ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
  if (!node){
    return NULL;
  }

  node->type = type;
  node->n = n;
  node->value = NULL;

  if (n > 0){
    node->children = (ASTNode**)malloc(n * sizeof(ASTNode*));
      if (!node->children) {
      free(node);
      return NULL;
    }

    va_list args;
    va_start(args, n);
    for (int i = 0; i < n; i++) {
        node->children[i] = va_arg(args, ASTNode*);
    }
    va_end(args);
  }

  else {
    node->children = NULL;
    node->value = value;
  }

  return node;
}

void printAST(ASTNode* node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) {
        if (i == depth - 1) {
            printf("|- ");
        } else {
            printf("|  ");
        }
    }

    if (node->n == 0 && node->value) {
        printf("%s\n", node->value);
    } else {
        switch (node->type) {
            case TERMINAL:    printf("TERMINAL\n"); break;
            case MACHINE:     printf("MACHINE\n"); break;
            case STATEMENT:   printf("STATEMENT\n"); break;
            case SYMLIST:     printf("SYMLIST\n"); break;
            case SYMBOLS:     printf("SYMBOLS\n"); break;
            case BLK:         printf("BLK\n"); break;
            case TRANSITIONS: printf("TRANSITIONS\n"); break;
            case ACTIONS:     printf("ACTIONS\n"); break;
            case ACTION:      printf("ACTION\n"); break;
            case STRING:      printf("STRING\n"); break;
            default:          printf("UNKNOWN\n"); break;
        }
        for (int i = 0; i < node->n; i++) {
            printAST(node->children[i], depth + 1);
        }
    }
}
