#include "../include/machine.h"
#include "../include/templates.h"
#include <stdio.h>
#include <string.h>

static void jsonChar(FILE *out, unsigned char c) {
  switch (c) {
  case '"':
    fputs("\\\"", out);
    break;
  case '\\':
    fputs("\\\\", out);
    break;
  case '<':
    fputs("\\u003c", out);
    break;
  case '>':
    fputs("\\u003e", out);
    break;
  case '&':
    fputs("\\u0026", out);
    break;
  default:
    if (c < 0x20 || c == 0x7f)
      fprintf(out, "\\u%04x", c);
    else
      fputc(c, out);
  }
}

static void jsonString(FILE *out, const char *s, int n) {
  fputc('"', out);
  for (int i = 0; i < n; i++)
    jsonChar(out, (unsigned char)s[i]);
  fputc('"', out);
}

static void jsonCString(FILE *out, const char *s) {
  jsonString(out, s, (int)strlen(s));
}

static const char *moveName(Move m) {
  return m == MOVE_LEFT ? "LEFT" : m == MOVE_RIGHT ? "RIGHT" : "STAY";
}

static void emitData(const Machine *m, FILE *out) {
  fputs("{\"name\":", out);
  jsonCString(out, m->name);
  fputs(",\"blank\":", out);
  jsonString(out, &m->blank, 1);
  fputs(",\"alphabet\":[", out);
  for (int i = 0; i < m->nalphabet; i++) {
    if (i)
      fputc(',', out);
    jsonString(out, &m->alphabet[i], 1);
  }
  fputs("],\"start\":", out);
  jsonCString(out, m->start);
  fputs(",\"input\":", out);
  jsonCString(out, m->input);
  fputs(",\"states\":[", out);
  for (int i = 0; i < m->nstates; i++) {
    const State *s = &m->states[i];
    if (i)
      fputc(',', out);
    fputs("\n{\"name\":", out);
    jsonCString(out, s->name);
    fputs(",\"rules\":[", out);
    for (int j = 0; j < s->nrules; j++) {
      const Rule *r = &s->rules[j];
      char write = r->write == WRITE_KEEP ? r->read : (char)r->write;
      if (j)
        fputc(',', out);
      fputs("\n{\"read\":", out);
      jsonString(out, &r->read, 1);
      fputs(",\"write\":", out);
      jsonString(out, &write, 1);
      fputs(",\"next\":", out);
      jsonCString(out, r->next);
      fputs(",\"move\":", out);
      jsonCString(out, moveName(r->move));
      fputc('}', out);
    }
    fputs("]}", out);
  }
  fputs("\n]}", out);
}

static void htmlEscape(FILE *out, const char *s) {
  for (; *s; s++) {
    switch (*s) {
    case '&':
      fputs("&amp;", out);
      break;
    case '<':
      fputs("&lt;", out);
      break;
    case '>':
      fputs("&gt;", out);
      break;
    case '"':
      fputs("&quot;", out);
      break;
    default:
      fputc(*s, out);
    }
  }
}

static int isNameChar(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '_';
}

static int emitKnownPlaceholder(const char *name, size_t len, const Machine *m,
                                FILE *out) {
  if (len == 5 && !strncmp(name, "TITLE", 5)) {
    htmlEscape(out, m->name);
    return 1;
  }
  if (len == 5 && !strncmp(name, "STYLE", 5)) {
    fputs(template_style_css, out);
    return 1;
  }
  if (len == 6 && !strncmp(name, "ENGINE", 6)) {
    fputs(template_engine_js, out);
    return 1;
  }
  if (len == 3 && !strncmp(name, "APP", 3)) {
    fputs(template_app_js, out);
    return 1;
  }
  if (len == 4 && !strncmp(name, "DATA", 4)) {
    emitData(m, out);
    return 1;
  }
  return 0;
}

/* Formatter-tolerant form: { <ws> { <ws> NAME [;] <ws> } <ws> } (e.g. when an
 * autoformatter splits {{NAME}}). Only known placeholder names are consumed. */
static const char *matchMangledPlaceholder(const char *p, const Machine *m,
                                           FILE *out) {
  const char *q = p;
  if (*q != '{')
    return NULL;
  q++;
  while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
    q++;
  if (*q != '{')
    return NULL;
  q++;
  while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
    q++;
  const char *name = q;
  while (isNameChar(*q))
    q++;
  size_t len = (size_t)(q - name);
  if (len == 0)
    return NULL;
  while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
    q++;
  if (*q == ';')
    q++;
  while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
    q++;
  if (*q != '}')
    return NULL;
  q++;
  while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
    q++;
  if (*q != '}')
    return NULL;
  q++;
  if (!emitKnownPlaceholder(name, len, m, out))
    return NULL;
  return q;
}

/* Copy the page template, replacing {{NAME}} placeholders in a single pass
 * so substituted content is never scanned again. Accepts formatter-mangled
 * placeholders (spaced braces / optional semicolon) for known names. */
void emitHtml(const Machine *m, FILE *out) {
  const char *p = template_index_html;
  while (*p) {
    const char *open = strstr(p, "{{");
    const char *mangled = NULL;
    /* Find the earliest mangled `{ <ws> {` candidate before a normal `{{`. */
    for (const char *s = p; *s; s++) {
      if (*s != '{')
        continue;
      const char *t = s + 1;
      while (*t == ' ' || *t == '\t' || *t == '\n' || *t == '\r')
        t++;
      if (*t == '{' && t != s + 1) {
        mangled = s;
        break;
      }
      if (*t == '{')
        break; /* this is a normal `{{`, handled by the strstr path */
    }

    if (open && (!mangled || open <= mangled)) {
      const char *close = strstr(open, "}}");
      if (!close) {
        fputs(p, out);
        break;
      }
      fwrite(p, 1, (size_t)(open - p), out);
      size_t len = (size_t)(close - open - 2);
      const char *name = open + 2;
      if (!emitKnownPlaceholder(name, len, m, out))
        fwrite(open, 1, (size_t)(close + 2 - open), out);
      p = close + 2;
      continue;
    }

    if (mangled) {
      fwrite(p, 1, (size_t)(mangled - p), out);
      const char *next = matchMangledPlaceholder(mangled, m, out);
      if (next)
        p = next;
      else {
        fputc(*mangled, out);
        p = mangled + 1;
      }
      continue;
    }

    fputs(p, out);
    break;
  }
}
