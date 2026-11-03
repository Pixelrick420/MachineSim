CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -Wall -Wextra -Iinclude -Ibuild -g
LDFLAGS =

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
BIN_DIR = bin
EXAMPLES_DIR = examples
TESTS_DIR = tests
TEMPLATES_DIR = templates
DIST_DIR = dist

# Compiler executable
COMPILER = $(BIN_DIR)/turing-compiler

# Source files
LEXER_SRC = $(SRC_DIR)/lexer.l
PARSER_SRC = $(SRC_DIR)/parser.y
LIB_SRC = $(SRC_DIR)/lib.c
CODEGEN_SRC = $(SRC_DIR)/codegen.c
MAIN_SRC = $(SRC_DIR)/main.c

# Page templates embedded into the compiler
TEMPLATE_FILES = $(TEMPLATES_DIR)/index.html $(TEMPLATES_DIR)/style.css \
                 $(TEMPLATES_DIR)/engine.js $(TEMPLATES_DIR)/app.js
TEMPLATES_GEN = $(BUILD_DIR)/templates.c
TEMPLATES_OBJ = $(BUILD_DIR)/templates.o

# Generated files
LEXER_GEN = $(BUILD_DIR)/lex.yy.c
PARSER_GEN_C = $(BUILD_DIR)/y.tab.c
PARSER_GEN_H = $(BUILD_DIR)/y.tab.h

# Object files
LEXER_OBJ = $(BUILD_DIR)/lex.yy.o
PARSER_OBJ = $(BUILD_DIR)/y.tab.o
LIB_OBJ = $(BUILD_DIR)/lib.o
CODEGEN_OBJ = $(BUILD_DIR)/codegen.o
MAIN_OBJ = $(BUILD_DIR)/main.o

# All objects
OBJS = $(LEXER_OBJ) $(PARSER_OBJ) $(LIB_OBJ) $(CODEGEN_OBJ) $(MAIN_OBJ) $(TEMPLATES_OBJ)

# Default target
.PHONY: all clean test install examples dist test-build help dev-server

all: $(COMPILER)

# Create directories
$(BUILD_DIR) $(BIN_DIR) $(DIST_DIR) $(TESTS_DIR):
	mkdir -p $@

# Generate lexer from .l file
$(LEXER_GEN): $(LEXER_SRC) $(PARSER_GEN_H) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

# Generate parser from .y file
$(PARSER_GEN_C) $(PARSER_GEN_H): $(PARSER_SRC) | $(BUILD_DIR)
	$(BISON) -d -o $(PARSER_GEN_C) $<

# Compile generated lexer
$(LEXER_OBJ): $(LEXER_GEN) $(PARSER_GEN_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile generated parser
$(PARSER_OBJ): $(PARSER_GEN_C) $(PARSER_GEN_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile library
$(LIB_OBJ): $(LIB_SRC) $(INCLUDE_DIR)/lib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile codegen
$(CODEGEN_OBJ): $(CODEGEN_SRC) $(INCLUDE_DIR)/lib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Embed templates as C string constants
$(TEMPLATES_GEN): $(TEMPLATE_FILES) | $(BUILD_DIR)
	@echo "GEN $@"
	@{ \
	for pair in index_html:index.html style_css:style.css engine_js:engine.js app_js:app.js; do \
		var=$${pair%%:*}; file=$(TEMPLATES_DIR)/$${pair#*:}; \
		echo "const char template_$$var[] ="; \
		sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/  "/' -e 's/$$/\\n"/' $$file; \
		echo ";"; \
	done; } > $@

$(TEMPLATES_OBJ): $(TEMPLATES_GEN)
	$(CC) $(CFLAGS) -Wno-overlength-strings -c $< -o $@

# Compile main
$(MAIN_OBJ): $(MAIN_SRC) $(INCLUDE_DIR)/lib.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link compiler
$(COMPILER): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Test with example (outputs JS to stdout)
test: $(COMPILER)
	@echo "=== Testing with hello-world.machine ==="
	@$(COMPILER) $(EXAMPLES_DIR)/hello-world.machine

# Build HTML visualization for an example
build-example: $(COMPILER) | $(DIST_DIR)
	@echo "=== Building visualization for hello-world ==="
	@$(COMPILER) $(EXAMPLES_DIR)/hello-world.machine > $(DIST_DIR)/machine.js
	@cp $(TEMPLATES_DIR)/index.html $(DIST_DIR)/
	@echo "Output: $(DIST_DIR)/index.html (open in browser)"

# Build all examples as separate visualizations
build-all: $(COMPILER) | $(DIST_DIR)
	@for f in $(EXAMPLES_DIR)/*.machine; do \
		base=$$(basename $$f .machine); \
		echo "Building $$base..."; \
		$(COMPILER) $$f > $(DIST_DIR)/$$base.js; \
		cp $(TEMPLATES_DIR)/index.html $(DIST_DIR)/$$base.html; \
		sed -i "s/machine\.js/$$base.js/" $(DIST_DIR)/$$base.html; \
		sed -i "s/Turing Machine Visualizer/$$base | Turing Visualizer/" $(DIST_DIR)/$$base.html; \
	done
	@echo "Output in $(DIST_DIR)/"

# Simple test compilation (for CI)
test-build: $(COMPILER)
	@echo "=== Testing compilation of all examples ==="
	@for f in $(EXAMPLES_DIR)/*.machine; do \
		echo "Compiling $$f..."; \
		$(COMPILER) $$f > /dev/null && echo "  OK" || (echo "  FAILED"; exit 1); \
	done
	@echo "All examples compiled successfully!"

# Run all examples (shows AST)
examples: $(COMPILER)
	@for f in $(EXAMPLES_DIR)/*.machine; do \
		echo "=== Compiling $$f ==="; \
		$(COMPILER) $$f; \
		echo ""; \
	done

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(DIST_DIR)

# Install to system (optional)
install: $(COMPILER)
	cp $(COMPILER) /usr/local/bin/turing-compiler

# Development server (requires python3)
dev-server: build-all
	@echo "Starting dev server at http://localhost:8000"
	@cd $(DIST_DIR) && python3 -m http.server 8000

# Help
help:
	@echo "Turing Machine Compiler - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build the compiler (default)"
	@echo "  test         - Build and test with default example (outputs JS to stdout)"
	@echo "  build-example - Build HTML visualization for hello-world example"
	@echo "  build-all    - Build visualizations for all examples in dist/"
	@echo "  test-build   - Quick compilation test for all examples (CI-friendly)"
	@echo "  examples     - Compile all examples (shows AST)"
	@echo "  dev-server   - Build all and start local HTTP server"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install compiler to /usr/local/bin"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Output: JavaScript with interactive HTML visualization"
	@echo ""
	@echo "Examples:"
	@echo "  make build-example"
	@echo "  make build-all"
	@echo "  make dev-server"
