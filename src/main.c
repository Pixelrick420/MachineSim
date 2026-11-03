#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/machine.h"

extern FILE* yyin;
extern int yyparse(void);
extern ASTNode* astRoot;

static void usage(FILE* to, const char* prog) {
    fprintf(to,
        "Usage: %s [-o output.html] <machine-file>\n"
        "\n"
        "Compiles a Turing machine definition into a single, self-contained\n"
        "HTML file. Without -o the page is written to standard output.\n",
        prog);
}

/* "examples/unary-add.machine" -> "unary-add" */
static char* displayName(const char* path) {
    const char* base = strrchr(path, '/');
    base = base ? base + 1 : path;
    char* name = strdup(base);
    char* dot = name ? strrchr(name, '.') : NULL;
    if (dot && dot != name)
        *dot = '\0';
    return name;
}

int main(int argc, char** argv) {
    const char* inPath = NULL;
    const char* outPath = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(stdout, argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "-o")) {
            if (++i >= argc) {
                usage(stderr, argv[0]);
                return 1;
            }
            outPath = argv[i];
        } else if (!inPath) {
            inPath = argv[i];
        } else {
            usage(stderr, argv[0]);
            return 1;
        }
    }
    if (!inPath) {
        usage(stderr, argv[0]);
        return 1;
    }

    FILE* f = fopen(inPath, "r");
    if (!f) {
        perror(inPath);
        return 1;
    }

    sourceName = inPath;
    yyin = f;
    int status = yyparse();
    fclose(f);
    if (status != 0)
        return 1;
    if (!astRoot) {
        reportError(0, "empty machine definition");
        return 1;
    }

    Machine m;
    if (buildMachine(astRoot, &m, displayName(inPath)) > 0)
        return 1;

    FILE* out = stdout;
    if (outPath) {
        out = fopen(outPath, "w");
        if (!out) {
            perror(outPath);
            return 1;
        }
    }
    emitHtml(&m, out);
    if (out != stdout && fclose(out) != 0) {
        perror(outPath);
        return 1;
    }
    return 0;
}
