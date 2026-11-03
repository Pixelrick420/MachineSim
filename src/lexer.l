%{
#include <stdio.h>
#include <string.h>
#include "../include/lib.h"
#include "y.tab.h"
%}

%option noyywrap

%%
STATE           { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return STATE; }
INPUT           { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return INPUT; }
BLANK           { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return BLANK; }
ALPHABET        { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return ALPHABET; }
WRITE           { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return WRITE; }
NEXT            { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return NEXT; }
MOVE            { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return MOVE; }
HALT            { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return HALT; }
START           { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return START; }
(LEFT|RIGHT)    { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return DIRECTION; }
\{              { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return LBRACE; }
\}              { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return RBRACE; }
\;              { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return ENDL; }
\:              { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return ASSIGN; }
\,              { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return COMMA; }
[a-z][a-zA-Z0-9]*  { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return IDENTIFIER; }
[01A-Z_+=!@#$%&-]   { yylval.node = createASTNode(TERMINAL, strdup(yytext), 0); return SYMBOL; }
"//".*           { /* skip comments */ }
[ \n\t]         { /* skip whitespace */ }
.               { /* ignore unknown */ }
%%