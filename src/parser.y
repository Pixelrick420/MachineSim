%{
#include <stdio.h>
#include <stdlib.h>
#include "../include/lib.h"

extern FILE* yyin;
extern int yylex();
extern int yyerror(char* s);

ASTNode* root;
char* startState = NULL;
%}

%union {
    ASTNode *node;
}

%type <node> MACHINE STATEMENT SYMLIST SYMBOLS BLK TRANSITIONS ACTIONS ACTION STRING
%token <node> NEXT MOVE BLANK ALPHABET WRITE ENDL STATE INPUT HALT COMMA RBRACE SYMBOL ASSIGN LBRACE IDENTIFIER DIRECTION START

%%
MACHINE     : MACHINE STATEMENT ENDL                                   { $$ = createASTNode(MACHINE, NULL, 3, $<node>1, $<node>2, $<node>3); root = $$; }
            | STATEMENT ENDL                                           { $$ = createASTNode(MACHINE, NULL, 2, $<node>1, $<node>2); root = $$; }
            ;

STATEMENT   : BLANK ASSIGN SYMBOL                                      { $$ = createASTNode(STATEMENT, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | ALPHABET ASSIGN SYMLIST                                  { $$ = createASTNode(STATEMENT, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | STATE IDENTIFIER ASSIGN BLK                              { $$ = createASTNode(STATEMENT, NULL, 4, $<node>1, $<node>2, $<node>3, $<node>4); }
            | INPUT ASSIGN STRING                                      { $$ = createASTNode(STATEMENT, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | START ASSIGN IDENTIFIER                                  { $$ = createASTNode(STATEMENT, NULL, 3, $<node>1, $<node>2, $<node>3); startState = $<node>3->value; }
            ;

SYMLIST     : LBRACE SYMBOLS RBRACE                                    { $$ = createASTNode(SYMLIST, NULL, 3, $<node>1, $<node>2, $<node>3); }
            ;

SYMBOLS     : SYMBOLS COMMA SYMBOL                                     { $$ = createASTNode(SYMBOLS, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | SYMBOL                                                   { $$ = createASTNode(SYMBOLS, NULL, 1, $<node>1); }
            ;

BLK         : LBRACE TRANSITIONS RBRACE                                { $$ = createASTNode(BLK, NULL, 3, $<node>1, $<node>2, $<node>3); }
            ;

TRANSITIONS : TRANSITIONS SYMBOL ASSIGN LBRACE ACTIONS RBRACE ENDL     { $$ = createASTNode(TRANSITIONS, NULL, 7, $<node>1, $<node>2, $<node>3, $<node>4, $<node>5, $<node>6, $<node>7); }
            | SYMBOL ASSIGN LBRACE ACTIONS RBRACE ENDL                 { $$ = createASTNode(TRANSITIONS, NULL, 6, $<node>1, $<node>2, $<node>3, $<node>4, $<node>5, $<node>6); }
            ;

ACTIONS     : ACTIONS COMMA ACTION                                     { $$ = createASTNode(ACTIONS, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | ACTION                                                   { $$ = createASTNode(ACTIONS, NULL, 1, $<node>1); }
            ;

ACTION      : WRITE ASSIGN SYMBOL                                      { $$ = createASTNode(ACTION, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | NEXT ASSIGN IDENTIFIER                                   { $$ = createASTNode(ACTION, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | NEXT ASSIGN HALT                                         { $$ = createASTNode(ACTION, NULL, 3, $<node>1, $<node>2, $<node>3); }
            | MOVE ASSIGN DIRECTION                                    { $$ = createASTNode(ACTION, NULL, 3, $<node>1, $<node>2, $<node>3); }
            ;

STRING      : STRING SYMBOL                                            { $$ = createASTNode(STRING, NULL, 2, $<node>1, $<node>2); }
            | SYMBOL                                                   { $$ = createASTNode(STRING, NULL, 1, $<node>1); }
            | /* empty */                                              { $$ = createASTNode(STRING, NULL, 0); }
            %%

int yyerror(char* s){
    fprintf(stderr, "Invalid Syntax: %s\n", s);
    exit(1);
}

int main(int argc, char** argv){
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <machine-file>\n", argv[0]);
        return 1;
    }
    
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror("Failed to open file");
        return 1;
    }
    
    yyin = f;
    
    if(yyparse() == 0){
        // Generate code from AST
        extern void generateCode(ASTNode* root, char* startState);
        generateCode(root, startState);
    }
    
    fclose(f);
    return 0;
}