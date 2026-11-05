# Machine language reference

A machine file describes a deterministic, single-tape Turing machine. The
compiler turns it into one self-contained HTML page. This document is the
complete reference for the file format. For a quick tour, see the
[README](../README.md).

- [A first look](#a-first-look)
- [Lexical rules](#lexical-rules)
- [Statements](#statements)
- [Transitions](#transitions)
- [How a machine runs](#how-a-machine-runs)
- [Worked example](#worked-example)
- [Diagnostics](#diagnostics)
- [Grammar](#grammar)

## A first look

```
// Adds two unary numbers: 111+11= becomes 11111
BLANK    : _;
ALPHABET : { 1 , + , = , _ };
START    : findPlus;

STATE findPlus : {
    1 : { WRITE : 1 , NEXT : findPlus , MOVE : RIGHT } ;
    + : { WRITE : 1 , NEXT : findEqual , MOVE : RIGHT } ;
    _ : { WRITE : _ , NEXT : HALT } ;
} ;

// ... more states ...

INPUT : 111+11=;
```

A file is a list of statements. Every statement ends with `;`. Statements can
appear in any order, and whitespace, including newlines, is not significant.

## Lexical rules

**Comments** start with `//` and run to the end of the line. They are not
recognized inside the body of an `INPUT` statement, where the `/` would be read
as a tape symbol.

**Keywords** are written in upper case and are case sensitive: `BLANK`,
`ALPHABET`, `START`, `STATE`, `INPUT`, `WRITE`, `MOVE`, `NEXT`, `HALT`, `LEFT`,
`RIGHT`, `STAY`. Any other upper case word of two or more characters is
rejected as an unknown keyword, so `AB` can never be a symbol.

**State names** start with a lowercase letter and continue with letters and
digits, so `findPlus`, `q0` and `carry2` are valid. Underscores and a leading
upper case letter are not allowed. `HALT` is reserved.

**Symbols** are single ASCII characters. In `BLANK`, `ALPHABET` and transitions
a symbol is one of:

| Kind | Characters |
| --- | --- |
| digit | `0` to `9` |
| upper case letter | `A` to `Z` |
| single lower case letter | `a` to `z` |
| punctuation | `_ + = ! @ # $ % & * < > \| ~ ^ ? . ( ) [ ] -` |

Lower case symbols are single characters only. `ab` is read as a state name and
reported as `symbol 'ab' must be a single character`.

## Statements

| Statement | Meaning | If omitted |
| --- | --- | --- |
| `BLANK : s;` | The symbol on every cell that has not been written. | Warning, uses `_`. |
| `ALPHABET : { a , b , c };` | Every symbol the machine can read or write. The blank symbol is always part of the alphabet, even when you leave it out. | No symbols beyond the blank. |
| `START : name;` | The state the machine starts in. | Warning, uses the first state. |
| `STATE name : { ... };` | Defines a state and its transitions. See [Transitions](#transitions). | Error if there are no states at all. |
| `INPUT : abc;` | The initial tape contents. | The tape starts empty. |

`BLANK`, `ALPHABET`, `START` and `INPUT` may each appear at most once. A state
name may be defined only once.

### INPUT

After `INPUT :` the compiler reads the tape character by character up to the
next `;`. That is why lower case letters and digits work there without any
quoting, and why whitespace and line breaks inside the body are skipped. Every
character must be in the alphabet and must be ASCII.

The first character is placed on cell 0 and the head starts there. An empty
`INPUT : ;` gives a blank tape, which is what `busy-beaver` uses.

## Transitions

A state is a set of transitions, one per symbol it can read:

```
STATE carry : {
    1 : { WRITE : 0 , MOVE : LEFT , NEXT : carry } ;
    0 : { WRITE : 1 , NEXT : HALT } ;
    _ : { WRITE : 1 , NEXT : HALT } ;
} ;
```

Each transition is `symbol : { actions } ;`. The actions are separated by
commas and may be given in any order, with at least one present:

| Action | Value | If omitted |
| --- | --- | --- |
| `WRITE : s` | Symbol to write on the current cell. | Keep the symbol that was read. |
| `MOVE : d` | `LEFT`, `RIGHT` or `STAY`. | `STAY` |
| `NEXT : name` | The next state, or `HALT` to stop. | `HALT` |

Because `NEXT` defaults to `HALT`, `1 : { WRITE : 0 };` is a valid transition
that writes and stops. Giving the same action twice in one transition is an
error.

A state does not have to cover every symbol. Reading a symbol with no
transition leaves the machine **stuck**, which the page reports as its own
status rather than as a crash. The transitions table shows a dash for each
missing entry.

## How a machine runs

1. The tape is unbounded in both directions. Every cell starts as the blank
   symbol, then `INPUT` is written starting at cell 0.
2. The head starts on cell 0 in the `START` state.
3. Each step reads the symbol under the head, finds the transition for the
   current state and that symbol, writes, moves, and enters the next state.
4. The machine stops when it reaches `HALT`, or gets stuck.

In the page, the step counter, the state, the symbol being read and the
transition just applied are all shown live. A run ends in one of these
statuses:

| Status | Meaning |
| --- | --- |
| Halted | A transition went to `HALT`. |
| Stuck | The current state has no transition for the symbol under the head. |
| Tape limit | The tape would have grown past 2<sup>26</sup> (about 67 million) cells. |

The page keeps the last 2<sup>20</sup> (about a million) steps, which is how far
**Back** can undo.

## Worked example

This is `examples/unary-add.machine`. It adds two numbers written in unary,
`111+11=` for 3 + 2. The machine turns the `+` into a `1`, then removes the
last `1` to make up for it.

```
BLANK : _;
ALPHABET : { 1 , + , = , _ };
START : findPlus;
STATE findPlus : {
    1 : { WRITE : 1 , NEXT : findPlus , MOVE : RIGHT } ;
    + : { WRITE : 1 , NEXT : findEqual , MOVE : RIGHT } ;
    _ : { WRITE : _ , NEXT : HALT } ;
} ;
STATE findEqual : {
    1 : { WRITE : 1 , NEXT : findEqual , MOVE : RIGHT } ;
    = : { WRITE : _ , NEXT : cleanup , MOVE : LEFT } ;
    _ : { WRITE : _ , NEXT : HALT } ;
} ;
STATE cleanup : {
    1 : { WRITE : _ , NEXT : HALT } ;
    _ : { WRITE : _ , NEXT : HALT } ;
} ;
INPUT : 111+11=;
```

Run to completion it takes 8 steps:

| Step | State | Cell | Read | Write | Move | Next | Tape after |
| ---: | --- | ---: | :---: | :---: | --- | --- | --- |
| 1 | findPlus | 0 | `1` | `1` | RIGHT | findPlus | `111+11=` |
| 2 | findPlus | 1 | `1` | `1` | RIGHT | findPlus | `111+11=` |
| 3 | findPlus | 2 | `1` | `1` | RIGHT | findPlus | `111+11=` |
| 4 | findPlus | 3 | `+` | `1` | RIGHT | findEqual | `111111=` |
| 5 | findEqual | 4 | `1` | `1` | RIGHT | findEqual | `111111=` |
| 6 | findEqual | 5 | `1` | `1` | RIGHT | findEqual | `111111=` |
| 7 | findEqual | 6 | `=` | `_` | LEFT | cleanup | `111111_` |
| 8 | cleanup | 5 | `1` | `_` | STAY | HALT | `11111__` |

The five remaining `1`s are the answer. Steps 1 to 3 rewrite a `1` with itself,
so they do not flash on the tape. The first real write is step 4.

## Diagnostics

Messages look like `file:line: error: text`, or `file: error: text` when no
line applies. Errors stop compilation: no page is written and the exit status
is 1. Warnings are printed but the page is still produced.

### Errors

| Message | Cause |
| --- | --- |
| `unexpected character 'c'` | A character that is not part of the language. |
| `unknown keyword 'FOO'` | An upper case word that is not a keyword. |
| `expected ':' after INPUT, found 'c'` | `INPUT` was not followed by a colon. |
| `input symbols must be ASCII characters` | A non-ASCII character in an `INPUT` body. |
| `syntax error, ...` | The statement does not match the [grammar](#grammar). The text names what was found and what was expected. |
| `symbol 'ab' must be a single character` | A multi-character lower case symbol. |
| `BLANK is specified more than once` | Also `ALPHABET`, `START` and `INPUT`. |
| `state 'a' is defined more than once` | Two `STATE` statements share a name. |
| `state 'a' has two transitions for '1'` | A state lists the same read symbol twice. |
| `state 'a': an action is given more than once for '1'` | `WRITE`, `MOVE` or `NEXT` repeated in one transition. |
| `state 'a' jumps to undefined state 'b'` | `NEXT` names a state that does not exist. |
| `transition symbol 'x' is not in the alphabet` | A read symbol outside the alphabet. |
| `written symbol 'x' is not in the alphabet` | A `WRITE` symbol outside the alphabet. |
| `input symbol 'x' is not in the alphabet` | An `INPUT` character outside the alphabet. |
| `start state 'x' is not defined` | `START` names a state that does not exist. |
| `no states defined` | The file has no `STATE` statement. |

### Warnings

| Message | Cause |
| --- | --- |
| `no BLANK symbol given, using '_'` | `BLANK` is missing. |
| `no START state given, using 'a'` | `START` is missing, so the first state is used. |
| `state 'b' is unreachable` | No path from the start state reaches this state. |

## Grammar

Written in the notation of the project's parser, with the tokens in upper case.
`SYM` is a symbol or a lower case single-letter name, because both lex as
identifiers or symbols and the length is checked when the machine is built.

```
machine      = { statement ";" }
statement    = "BLANK" ":" sym
             | "ALPHABET" ":" "{" sym { "," sym } "}"
             | "START" ":" name
             | "STATE" name ":" "{" transition { transition } "}"
             | "INPUT" ":" { sym }

transition   = sym ":" "{" action { "," action } "}" ";"
action       = "WRITE" ":" sym
             | "NEXT"  ":" ( name | "HALT" )
             | "MOVE"  ":" ( "LEFT" | "RIGHT" | "STAY" )

name         = lower { letter | digit }
sym          = digit | UPPER | lower | punctuation
```

The source of truth is [`src/lexer.l`](../src/lexer.l) and
[`src/parser.y`](../src/parser.y).
