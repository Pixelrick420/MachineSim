# Turing Machine Compiler

A compiler that takes a Turing machine specification and an input string, and generates an interactive JavaScript visualization of the machine executing on that input.

## Project Structure

```
Turing/
├── Makefile              # Build system
├── README.md             # This file
├── include/
│   └── lib.h             # AST node definitions
├── src/
│   ├── lexer.l           # Flex lexer specification
│   ├── parser.y          # Bison parser specification
│   ├── lib.c             # AST utility functions
│   └── codegen.c         # JavaScript code generator
├── examples/
│   ├── hello-world.machine
│   ├── binary-increment.machine
│   ├── unary-add.machine
│   ├── palindrome.machine
│   └── busy-beaver.machine
├── templates/
│   └── index.html        # HTML template for visualization
├── build/                # Generated build artifacts (gitignored)
├── bin/                  # Compiled binaries (gitignored)
└── dist/                 # Generated visualizations (gitignored)
```

## Turing Machine Language

The language defines a Turing machine with the following syntax:

```
BLANK : <symbol>;
ALPHABET : { <symbol> , <symbol> , ... };
START : <state_name>;
STATE <state_name> : {
    <symbol> : { WRITE : <symbol> , NEXT : <state_name> , MOVE : <LEFT|RIGHT> } ;
    ...
} ;
INPUT : <string>;
```

### Example: Hello World

```
BLANK : $;
ALPHABET : { 0 , 1 , $ };
START : a;
STATE a : {
    0 : { WRITE : 0 , NEXT : b , MOVE : LEFT } ;
    1 : { WRITE : 0 , NEXT : b , MOVE : LEFT } ;
    $ : { WRITE : 0 , NEXT : b , MOVE : LEFT } ;
} ;
STATE b : {
    0 : { WRITE : 1 , NEXT : HALT } ;
    1 : { WRITE : 1 , NEXT : HALT } ;
    $ : { WRITE : 1 , NEXT : HALT } ;
} ;
INPUT : 001;
```

## Building

```bash
# Build the compiler
make

# Build and test with hello-world example
make test

# Build visualizations for all examples
make build-all

# Quick compilation test (CI-friendly)
make test-build

# Start development server
make dev-server

# Clean build artifacts
make clean
```

## Running the Compiler

```bash
# Compile a machine file to stdout (JavaScript)
./bin/turing-compiler examples/hello-world.machine

# Save output to a file
./bin/turing-compiler examples/hello-world.machine > machine.js
```

## Visualization

The compiler generates a self-contained JavaScript file that includes:
- The Turing machine configuration
- An interactive `TuringVisualizer` class
- Auto-initialization on DOM load

To view a visualization:
1. Run `make build-all` to generate HTML/JS files in `dist/`
2. Open any `.html` file in a browser
3. Or run `make dev-server` and visit `http://localhost:8000`

### Visualization Features

- **Step**: Execute one transition
- **Run**: Auto-step with adjustable speed
- **Stop**: Pause auto-stepping
- **Undo**: Go back one step (with history)
- **Reset**: Return to initial state
- **Speed control**: Adjust animation speed (50-2000ms)

The tape shows:
- **Head position** (▼ marker, highlighted cell)
- **Initial input** (darker background)
- **Cell indices** for reference

## Example Machines

| Machine | Description |
|---------|-------------|
| `hello-world` | Simple 2-state machine |
| `binary-increment` | Increments a binary number |
| `unary-add` | Adds two unary numbers (111+11=) |
| `palindrome` | Checks if binary input is a palindrome |
| `busy-beaver` | 5-state busy beaver (runs 47M+ steps!) |

## Requirements

- `gcc` - C compiler
- `flex` - Lexer generator
- `bison` - Parser generator
- `make` - Build system

## License

MIT