#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lib.h"

extern FILE* yyin;
extern int yyparse(void);
extern ASTNode* astRoot;
extern void generateCode(ASTNode* root, char* startState);

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <machine-file>\n", argv[0]);
        return 1;
    }

    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }

    sourceName = argv[1];
    yyin = f;

    int status = yyparse();
    fclose(f);
    if (status != 0)
        return 1;
    if (!astRoot) {
        reportError(0, "empty machine definition");
        return 1;
    }

    generateCode(astRoot, NULL);
    return 0;
}
