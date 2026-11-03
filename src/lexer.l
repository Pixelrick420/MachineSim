%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lib.h"
#include "y.tab.h"

#define TOKEN(t) do { \
    yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); \
    return t; \
} while (0)
%}

%option noyywrap yylineno nounput noinput

/* After the INPUT keyword the tape contents are read character by
 * character, so that lowercase letters and digits can be used as symbols. */
%x INPUT_COLON INPUT_BODY

WS      [ \t\r\n]+

%%

STATE                   { TOKEN(STATE); }
INPUT                   { BEGIN(INPUT_COLON); TOKEN(INPUT); }
BLANK                   { TOKEN(BLANK); }
ALPHABET                { TOKEN(ALPHABET); }
WRITE                   { TOKEN(WRITE); }
NEXT                    { TOKEN(NEXT); }
MOVE                    { TOKEN(MOVE); }
HALT                    { TOKEN(HALT); }
START                   { TOKEN(START); }
LEFT|RIGHT|STAY         { TOKEN(DIRECTION); }
"{"                     { TOKEN(LBRACE); }
"}"                     { TOKEN(RBRACE); }
";"                     { TOKEN(ENDL); }
":"                     { TOKEN(ASSIGN); }
","                     { TOKEN(COMMA); }
[a-z][a-zA-Z0-9]*       { TOKEN(IDENTIFIER); }
[0-9A-Z_+=!@#$%&*<>|~^?.()\[\]-] { TOKEN(SYMBOL); }
[A-Z_][A-Z0-9_]+        { lexError("unknown keyword '%s'", yytext); }
"//".*                  { /* comment */ }
{WS}                    { /* whitespace */ }
.                       { lexError("unexpected character '%s'", yytext); }

<INPUT_COLON>{
":"                     { BEGIN(INPUT_BODY); TOKEN(ASSIGN); }
{WS}                    { }
.                       { lexError("expected ':' after INPUT, found '%s'", yytext); }
}

<INPUT_BODY>{
";"                     { BEGIN(INITIAL); TOKEN(ENDL); }
{WS}                    { }
[\x80-\xff]             { lexError("input symbols must be ASCII characters"); }
.                       { TOKEN(SYMBOL); }
}

%%
