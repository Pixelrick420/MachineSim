#include "../include/machine.h"
#include <string.h>

static int errors;

static int inAlphabet(const Machine *m, char c) {
  return memchr(m->alphabet, c, m->nalphabet) != NULL;
}

static const State *findState(const Machine *m, const char *name) {
  for (int i = 0; i < m->nstates; i++)
    if (strcmp(m->states[i].name, name) == 0)
      return &m->states[i];
  return NULL;
}

static void err(int line, const char *what, char c) {
  reportError(line, "%s '%c' is not in the alphabet", what, c);
  errors++;
}

static void checkDirectives(const Machine *m) {
  struct {
    const char *name;
    int count, line;
  } d[] = {
      {"BLANK", m->blankCount, m->blankLine},
      {"ALPHABET", m->alphabetCount, m->alphabetLine},
      {"START", m->startCount, m->startLine},
      {"INPUT", m->inputCount, m->inputLine},
  };
  for (int i = 0; i < 4; i++) {
    if (d[i].count > 1) {
      reportError(d[i].line, "%s is specified more than once", d[i].name);
      errors++;
    }
  }
  if (m->blankCount == 0)
    reportWarning(0, "no BLANK symbol given, using '_'");
  if (m->startCount == 0 && m->nstates > 0)
    reportWarning(0, "no START state given, using '%s'", m->states[0].name);
}

static void checkState(const Machine *m, int index) {
  const State *s = &m->states[index];

  for (int i = 0; i < index; i++) {
    if (strcmp(m->states[i].name, s->name) == 0) {
      reportError(s->line, "state '%s' is defined more than once", s->name);
      errors++;
    }
  }

  for (int j = 0; j < s->nrules; j++) {
    const Rule *r = &s->rules[j];

    if (r->duplicateActions) {
      reportError(r->line,
                  "state '%s': an action is given more than once for '%c'",
                  s->name, r->read);
      errors++;
    }
    for (int k = 0; k < j; k++) {
      if (s->rules[k].read == r->read) {
        reportError(r->line, "state '%s' has two transitions for '%c'", s->name,
                    r->read);
        errors++;
      }
    }
    if (!inAlphabet(m, r->read))
      err(r->line, "transition symbol", r->read);
    if (r->write != WRITE_KEEP && !inAlphabet(m, (char)r->write))
      err(r->line, "written symbol", (char)r->write);
    if (strcmp(r->next, "HALT") != 0 && !findState(m, r->next)) {
      reportError(r->line, "state '%s' jumps to undefined state '%s'", s->name,
                  r->next);
      errors++;
    }
  }
}

static void checkReachable(const Machine *m) {
  if (m->nstates == 0)
    return;
  int seen[m->nstates];
  int stack[m->nstates];
  int top = 0;
  memset(seen, 0, sizeof seen);

  const State *start = findState(m, m->start);
  if (!start)
    return;
  seen[start - m->states] = 1;
  stack[top++] = (int)(start - m->states);
  while (top > 0) {
    const State *s = &m->states[stack[--top]];
    for (int j = 0; j < s->nrules; j++) {
      const State *n = findState(m, s->rules[j].next);
      if (n && !seen[n - m->states]) {
        seen[n - m->states] = 1;
        stack[top++] = (int)(n - m->states);
      }
    }
  }
  for (int i = 0; i < m->nstates; i++)
    if (!seen[i])
      reportWarning(m->states[i].line, "state '%s' is unreachable",
                    m->states[i].name);
}

int validateMachine(const Machine *m) {
  errors = 0;
  checkDirectives(m);

  if (m->nstates == 0) {
    reportError(0, "no states defined");
    return ++errors;
  }
  if (!findState(m, m->start)) {
    reportError(m->startLine, "start state '%s' is not defined", m->start);
    errors++;
  }
  for (int i = 0; i < m->nstates; i++)
    checkState(m, i);
  for (int i = 0; m->input[i]; i++) {
    if (!inAlphabet(m, m->input[i])) {
      reportError(m->inputLine, "input symbol '%c' is not in the alphabet",
                  m->input[i]);
      errors++;
      break;
    }
  }
  if (errors == 0)
    checkReachable(m);
  return errors;
}
