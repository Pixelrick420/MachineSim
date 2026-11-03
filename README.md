# Turing Machine Compiler

Compiles a Turing machine specification and input string into a single,
self-contained HTML file. Open the file in any browser to watch the machine run.

## Quick start

```bash
make                                   # build the compiler
make build-all                         # dist/<example>.html for every example
open dist/unary-add.html               # or double-click it, no server needed

./bin/turing-compiler -o out.html examples/palindrome.machine
./bin/turing-compiler examples/palindrome.machine > out.html   # stdout works too
```

## Project structure

```
include/   lib.h (AST), machine.h (machine model), templates.h
src/       lexer.l, parser.y, lib.c, machine.c (AST -> model),
           validate.c (semantic checks), codegen.c (HTML output), main.c
templates/ index.html, style.css, engine.js, app.js (embedded into the compiler)
examples/  sample machines
tests/     check.js and error cases, run with `make test`
```

The templates are embedded into the compiler binary at build time, so the
generated page needs nothing else: no scripts, styles or fonts are fetched.

## Language

```
BLANK : <symbol>;
ALPHABET : { <symbol> , <symbol> , ... };
START : <state>;
STATE <state> : {
    <symbol> : { WRITE : <symbol> , NEXT : <state|HALT> , MOVE : <LEFT|RIGHT|STAY> } ;
    ...
} ;
INPUT : <symbols>;
// comments run to the end of the line
```

- A symbol is one character: a digit, an uppercase letter, a single lowercase
  letter, or one of `_ + = ! @ # $ % & * < > | ~ ^ ? . ( ) [ ]`.
- Everything after `INPUT :` up to `;` is read character by character, so
  `INPUT : abba;` works. Whitespace is ignored.
- In a transition every action is optional. `WRITE` defaults to the symbol read,
  `MOVE` to `STAY` and `NEXT` to `HALT`.
- `BLANK` defaults to `_` and is always part of the alphabet. `START` defaults
  to the first state. A missing transition makes the machine stop as "stuck".

The compiler reports errors with file and line, for example undefined states,
symbols outside the alphabet, duplicate states or transitions, and a missing
start state. It also warns about unreachable states.

## The visualization

- Tape slides under a fixed head; writes flash and the active transition is
  highlighted in the table.
- Play, pause, step, step back (up to 1M steps), reset.
- Speed from 0.5 steps/s to "max", which runs millions of steps per second
  (the 47M step busy beaver finishes in a few seconds).
- Edit the input in the page and re-run without recompiling.
- Light and dark themes, keyboard shortcuts (space, arrows, R), mobile layout.

## Examples

| Machine | Description |
|---------|-------------|
| `hello-world` | Simple 2-state machine |
| `binary-increment` | Increments a binary number |
| `unary-add` | Adds two unary numbers (`111+11=` gives five 1s) |
| `palindrome` | Writes 1 left of the input if it is a binary palindrome, else 0 |
| `busy-beaver` | 5-state champion, halts after 47,176,870 steps |

## Requirements

`gcc`, `flex`, `bison`, `make`. Tests also need `node`.

## License

MIT
