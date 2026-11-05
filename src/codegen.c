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

/* Copy the page template, replacing {{NAME}} placeholders in a single pass
 * so substituted content is never scanned again. */
void emitHtml(const Machine *m, FILE *out) {
  const char *p = template_index_html;
  while (*p) {
    const char *open = strstr(p, "{{");
    const char *close = open ? strstr(open, "}}") : NULL;
    if (!open || !close) {
      fputs(p, out);
      break;
    }
    fwrite(p, 1, (size_t)(open - p), out);
    size_t len = (size_t)(close - open - 2);
    const char *name = open + 2;
    if (len == 5 && !strncmp(name, "TITLE", len))
      htmlEscape(out, m->name);
    else if (len == 5 && !strncmp(name, "STYLE", len))
      fputs(template_style_css, out);
    else if (len == 6 && !strncmp(name, "ENGINE", len))
      fputs(template_engine_js, out);
    else if (len == 3 && !strncmp(name, "APP", len))
      fputs(template_app_js, out);
    else if (len == 4 && !strncmp(name, "DATA", len))
      emitData(m, out);
    else
      fwrite(open, 1, (size_t)(close + 2 - open), out);
    p = close + 2;
  }
}
