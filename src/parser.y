%{
#include <stdio.h>
#include <stdlib.h>
#include "../include/lib.h"

extern int yylex(void);
extern int yylineno;
int yyerror(const char* s);

ASTNode* astRoot = NULL;
%}

%define parse.error verbose

%union {
    ASTNode *node;
}

%type <node> MACHINE STATEMENT SYMLIST SYMBOLS BLK TRANSITIONS ACTIONS ACTION STRING SYM
%token <node> NEXT MOVE BLANK ALPHABET WRITE STATE INPUT HALT SYMBOL IDENTIFIER DIRECTION START
%token <node> ENDL ";" ASSIGN ":" COMMA "," LBRACE "{" RBRACE "}"

%%
MACHINE     : MACHINE STATEMENT ENDL                                   { $$ = createASTNode(MACHINE, NULL, 3, $1, $2, $3); astRoot = $$; }
            | STATEMENT ENDL                                           { $$ = createASTNode(MACHINE, NULL, 2, $1, $2); astRoot = $$; }
            ;

STATEMENT   : BLANK ASSIGN SYM                                         { $$ = createASTNode(STATEMENT, NULL, 3, $1, $2, $3); }
            | ALPHABET ASSIGN SYMLIST                                  { $$ = createASTNode(STATEMENT, NULL, 3, $1, $2, $3); }
            | STATE IDENTIFIER ASSIGN BLK                              { $$ = createASTNode(STATEMENT, NULL, 4, $1, $2, $3, $4); }
            | INPUT ASSIGN STRING                                      { $$ = createASTNode(STATEMENT, NULL, 3, $1, $2, $3); }
            | START ASSIGN IDENTIFIER                                  { $$ = createASTNode(STATEMENT, NULL, 3, $1, $2, $3); }
            ;

SYM         : SYMBOL                                                   { $$ = $1; }
            | IDENTIFIER                                               { $$ = $1; }
            ;

SYMLIST     : LBRACE SYMBOLS RBRACE                                    { $$ = createASTNode(SYMLIST, NULL, 3, $1, $2, $3); }
            ;

SYMBOLS     : SYMBOLS COMMA SYM                                        { $$ = createASTNode(SYMBOLS, NULL, 3, $1, $2, $3); }
            | SYM                                                      { $$ = createASTNode(SYMBOLS, NULL, 1, $1); }
            ;

BLK         : LBRACE TRANSITIONS RBRACE                                { $$ = createASTNode(BLK, NULL, 3, $1, $2, $3); }
            ;

TRANSITIONS : TRANSITIONS SYM ASSIGN LBRACE ACTIONS RBRACE ENDL        { $$ = createASTNode(TRANSITIONS, NULL, 7, $1, $2, $3, $4, $5, $6, $7); }
            | SYM ASSIGN LBRACE ACTIONS RBRACE ENDL                    { $$ = createASTNode(TRANSITIONS, NULL, 6, $1, $2, $3, $4, $5, $6); }
            ;

ACTIONS     : ACTIONS COMMA ACTION                                     { $$ = createASTNode(ACTIONS, NULL, 3, $1, $2, $3); }
            | ACTION                                                   { $$ = createASTNode(ACTIONS, NULL, 1, $1); }
            ;

ACTION      : WRITE ASSIGN SYM                                         { $$ = createASTNode(ACTION, NULL, 3, $1, $2, $3); }
            | NEXT ASSIGN IDENTIFIER                                   { $$ = createASTNode(ACTION, NULL, 3, $1, $2, $3); }
            | NEXT ASSIGN HALT                                         { $$ = createASTNode(ACTION, NULL, 3, $1, $2, $3); }
            | MOVE ASSIGN DIRECTION                                    { $$ = createASTNode(ACTION, NULL, 3, $1, $2, $3); }
            ;

STRING      : STRING SYM                                               { $$ = createASTNode(STRING, NULL, 2, $1, $2); }
            | /* empty */                                              { $$ = createASTNode(STRING, NULL, 0); }
            ;
%%

int yyerror(const char* s) {
    fatalAt(yylineno, "%s", s);
    return 1;
}
