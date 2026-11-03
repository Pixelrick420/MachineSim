#ifndef MACHINE_H
#define MACHINE_H

#include "lib.h"

typedef enum { MOVE_LEFT = -1, MOVE_STAY = 0, MOVE_RIGHT = 1 } Move;

#define WRITE_KEEP (-1)

typedef struct {
  char read;
  int write;      /* symbol to write, or WRITE_KEEP to leave the cell as is */
  char* next;     /* state name or "HALT" */
  Move move;
  int line;
  int duplicateActions; /* an action (WRITE/NEXT/MOVE) was given twice */
} Rule;

typedef struct {
  char* name;
  Rule* rules;
  int nrules;
  int line;
} State;

typedef struct {
  char* name;            /* display name, taken from the file name */

  char blank;
  char* alphabet;        /* not NUL terminated, see nalphabet */
  int nalphabet;
  char* start;
  char* input;           /* NUL terminated */
  State* states;
  int nstates;

  /* How many times each directive appeared, and where. */
  int blankCount, alphabetCount, startCount, inputCount;
  int blankLine, alphabetLine, startLine, inputLine;
} Machine;

/* Flatten the AST into a Machine. Returns the number of errors reported. */
int buildMachine(ASTNode* root, Machine* m, const char* name);


/* Write the self-contained HTML visualization for the machine. */
void emitHtml(const Machine* m, FILE* out);

#endif
