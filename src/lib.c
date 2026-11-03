#include <stdio.h>
#include <string.h>
#include "../include/lib.h"

extern int yylineno;

const char* sourceName = "<input>";

static void vreport(const char* kind, int line, const char* fmt, va_list args) {
  fprintf(stderr, "%s:", sourceName);
  if (line > 0)
    fprintf(stderr, "%d:", line);
  fprintf(stderr, " %s: ", kind);
  vfprintf(stderr, fmt, args);
  fputc('\n', stderr);
}

void reportError(int line, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vreport("error", line, fmt, args);
  va_end(args);
}

void reportWarning(int line, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vreport("warning", line, fmt, args);
  va_end(args);
}

void fatalAt(int line, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vreport("error", line, fmt, args);
  va_end(args);
  exit(1);
}

ASTNode* createASTNode(NodeType type, char* value, int n, ...){
  ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
  if (!node){
    return NULL;
  }

  node->type = type;
  node->n = n;
  node->value = NULL;
  node->line = yylineno;

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
    if (node->children[0])
      node->line = node->children[0]->line;
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
