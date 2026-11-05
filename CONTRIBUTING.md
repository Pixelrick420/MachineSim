# Contributing

Thanks for taking a look. MachineSim is small on purpose: a C compiler, four
template files and two test scripts. This guide covers how to build it, what a
good change looks like, and the few rules worth knowing before you touch the
interface.

## Setup

You need a C compiler, `make`, `flex`, `bison` and a recent Node.js. Node is used
only to run the tests, and CI runs them on Node 22.

```sh
# Debian and Ubuntu
sudo apt install build-essential flex bison nodejs

# macOS
brew install flex bison node
```

Then:

```sh
make          # builds bin/turing-compiler
make test     # runs every check
```

The `bison` that ships with macOS is very old. If `make` fails on the grammar,
put the Homebrew one first with `export PATH="$(brew --prefix bison)/bin:$PATH"`.

## Working on the compiler

The pipeline is lexer, parser, model, validation, then code generation. Start
with [docs/architecture.md](docs/architecture.md) to see which file owns what.

- A new keyword or syntax change touches `src/lexer.l` and `src/parser.y`.
- A new semantic check goes in `src/validate.c`, and reports through
  `reportError` or `reportWarning` so it gets the `file:line:` prefix.
- Update [docs/language.md](docs/language.md) in the same change. Its
  diagnostics tables are meant to list every message the compiler can print.

### Add a diagnostic test

Every error has a case in `tests/errors/`. Create a `.machine` file whose first
line states what the message must contain:

```
// expect: not in the alphabet
BLANK : _;
...
```

`tests/check.js` compiles each file and requires a non-zero exit, no output on
stdout and that text in the error.

### Add an example

1. Put the machine in `examples/`, with a comment on the first line saying what
   it does.
2. Add a check to `tests/check.js` that runs it and asserts the final tape. The
   existing ones show the pattern.
3. Add a row to the examples table in the README.

## Working on the interface

The page is `templates/index.html`, `style.css`, `engine.js` and `app.js`. They
are embedded into the compiler at build time, so run `make` after editing them.
To look at your change, compile an example and open the result:

```sh
make build-example EXAMPLE=palindrome   # writes dist/palindrome.html
```

The interface follows the
[better-ui](https://www.ui-skills.com/skills/jakubkrehel/better-ui) principles,
and `tests/ui.js` enforces the ones that can be checked from the CSS. The
short version:

- **Depth comes from a shadow ring**, `var(--shadow-border)`, not a 1px border.
  Keep real borders on inputs, dividers and chips.
- **Name your transition properties.** Never `transition: all`.
- **Gate hover** inside `@media (hover: hover)`.
- **Gate movement, scale and blur** inside
  `@media (prefers-reduced-motion: no-preference)`, and leave an opacity fade
  for everyone else.
- **Use transitions for state changes**, because they can be interrupted.
  Keyframes are for sequences that run once.
- **Draw icons with `currentColor`**, with a 2px stroke on an 18px grid.

The page must stay a single file with nothing to fetch. A `data:` URI is fine,
a link to a CDN is not. `tests/check.js` also fails if the word `gradient`
appears anywhere in the page.

If you change how the page looks, regenerate the README screenshots:

```sh
npm install --no-save playwright && npx playwright install chromium
node scripts/screenshots.js
```

## Commits

Use a short, lower case summary prefixed by what kind of change it is:

| Prefix | For |
| --- | --- |
| `feat:` | something a user can see or use that was not there before |
| `fix:` | something that behaved wrongly and now does not |
| `chore:` | docs, tests, tooling and cleanup |

For example: `feat: go to line`, `fix: tts playback`, `chore: update README`.
Keep each commit to one idea and make sure `make test` passes on it, so the
history stays easy to bisect. Use the body of the message to explain why.

## Before you open a pull request

- `make test` passes from a clean tree (`make clean && make && make test`).
- Docs and tests changed alongside the code.
- Interface changes checked in light and dark, on a narrow screen, and with
  reduced motion turned on.

## License

By contributing you agree that your work is released under the project's
license. See the README.
