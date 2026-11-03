#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/machine.h"

static int errors;

static char* xstrdup(const char* s) {
  char* p = strdup(s);
  if (!p) { perror("strdup"); exit(1); }
  return p;
}

static void* xrealloc(void* p, size_t n) {
  void* q = realloc(p, n);
  if (!q) { perror("realloc"); exit(1); }
  return q;
}

/* A symbol is exactly one character. */
static char symbolOf(ASTNode* n) {
  if (strlen(n->value) != 1) {
    reportError(n->line, "symbol '%s' must be a single character", n->value);
    errors++;
    return '?';
  }
  return n->value[0];
}

static void collectAlphabet(Machine* m, ASTNode* node) {
  if (node->n == 3)
    collectAlphabet(m, node->children[0]);
  ASTNode* sym = node->children[node->n - 1];
  m->alphabet = xrealloc(m->alphabet, m->nalphabet + 1);
  m->alphabet[m->nalphabet++] = symbolOf(sym);
}

static void collectInput(Machine* m, ASTNode* node, int* len) {
  if (node->n == 0)
    return;
  collectInput(m, node->children[0], len);
  m->input = xrealloc(m->input, *len + 2);
  m->input[(*len)++] = symbolOf(node->children[1]);
  m->input[*len] = '\0';
}

static void collectActions(ASTNode* node, Rule* r) {
  if (node->n == 3)
    collectActions(node->children[0], r);
  ASTNode* action = node->children[node->n - 1];
  const char* kind = action->children[0]->value;
  ASTNode* value = action->children[2];
  if (strcmp(kind, "WRITE") == 0) {
    if (r->write != WRITE_KEEP) r->duplicateActions = 1;
    r->write = (unsigned char)symbolOf(value);
  } else if (strcmp(kind, "NEXT") == 0) {
    if (r->next) r->duplicateActions = 1;
    r->next = xstrdup(value->value);
  } else {
    if (r->move != MOVE_STAY) r->duplicateActions = 1;
    r->move = strcmp(value->value, "LEFT") == 0 ? MOVE_LEFT
            : strcmp(value->value, "RIGHT") == 0 ? MOVE_RIGHT : MOVE_STAY;
  }
}

static void collectTransitions(State* s, ASTNode* node) {
  int base = 0;
  if (node->n == 7) {
    collectTransitions(s, node->children[0]);
    base = 1;
  }
  Rule r = { 0, WRITE_KEEP, NULL, MOVE_STAY, node->children[base]->line, 0 };
  r.read = symbolOf(node->children[base]);
  collectActions(node->children[base + 3], &r);
  if (!r.next)
    r.next = xstrdup("HALT");
  s->rules = xrealloc(s->rules, (s->nrules + 1) * sizeof(Rule));
  s->rules[s->nrules++] = r;
}

static void collectStatement(Machine* m, ASTNode* stmt) {
  const char* kw = stmt->children[0]->value;
  if (strcmp(kw, "BLANK") == 0) {
    m->blank = symbolOf(stmt->children[2]);
    if (m->blankCount++ == 0) m->blankLine = stmt->line;
  } else if (strcmp(kw, "ALPHABET") == 0) {
    if (m->alphabetCount++ == 0) m->alphabetLine = stmt->line;
    free(m->alphabet);
    m->alphabet = NULL;
    m->nalphabet = 0;
    collectAlphabet(m, stmt->children[2]->children[1]);
  } else if (strcmp(kw, "START") == 0) {
    free(m->start);
    m->start = xstrdup(stmt->children[2]->value);
    if (m->startCount++ == 0) m->startLine = stmt->line;
  } else if (strcmp(kw, "INPUT") == 0) {
    int len = 0;
    free(m->input);
    m->input = NULL;
    if (m->inputCount++ == 0) m->inputLine = stmt->line;
    collectInput(m, stmt->children[2], &len);
  } else if (strcmp(kw, "STATE") == 0) {
    m->states = xrealloc(m->states, (m->nstates + 1) * sizeof(State));
    State* s = &m->states[m->nstates++];
    s->name = xstrdup(stmt->children[1]->value);
    s->line = stmt->line;
    s->rules = NULL;
    s->nrules = 0;
    collectTransitions(s, stmt->children[3]->children[1]);
  }
}

static void walk(Machine* m, ASTNode* node) {
  if (node->type == STATEMENT) {
    collectStatement(m, node);
    return;
  }
  for (int i = 0; i < node->n; i++)
    if (node->children[i]->type == MACHINE || node->children[i]->type == STATEMENT)
      walk(m, node->children[i]);
}

int buildMachine(ASTNode* root, Machine* m, const char* name) {
  memset(m, 0, sizeof *m);
  m->name = xstrdup(name);
  m->blank = '_';
  errors = 0;

  walk(m, root);

  /* Defaults for anything left unspecified. */
  if (!m->input)
    m->input = xstrdup("");
  if (!m->start && m->nstates > 0)
    m->start = xstrdup(m->states[0].name);
  if (!m->start)
    m->start = xstrdup("");

  /* The blank symbol is always part of the alphabet. */
  if (!memchr(m->alphabet ? m->alphabet : "", m->blank, m->nalphabet)) {
    m->alphabet = xrealloc(m->alphabet, m->nalphabet + 1);
    m->alphabet[m->nalphabet++] = m->blank;
  }
  return errors;
}
