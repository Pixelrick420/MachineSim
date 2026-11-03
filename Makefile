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
TEMPLATES_DIR = templates
DIST_DIR = dist

# Compiler executable
COMPILER = $(BIN_DIR)/turing-compiler

# Source files
LEXER_SRC = $(SRC_DIR)/lexer.l
PARSER_SRC = $(SRC_DIR)/parser.y
LIB_SRC = $(SRC_DIR)/lib.c
CODEGEN_SRC = $(SRC_DIR)/codegen.c
MACHINE_SRC = $(SRC_DIR)/machine.c
VALIDATE_SRC = $(SRC_DIR)/validate.c
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
MACHINE_OBJ = $(BUILD_DIR)/machine.o
VALIDATE_OBJ = $(BUILD_DIR)/validate.o
MAIN_OBJ = $(BUILD_DIR)/main.o

# All objects
OBJS = $(LEXER_OBJ) $(PARSER_OBJ) $(LIB_OBJ) $(CODEGEN_OBJ) $(MACHINE_OBJ) $(VALIDATE_OBJ) $(MAIN_OBJ) $(TEMPLATES_OBJ)

# Default target
.PHONY: all clean test test-build build-all build-example install help

all: $(COMPILER)

# Create directories
$(BUILD_DIR) $(BIN_DIR) $(DIST_DIR):
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

# Compile codegen and machine model
$(CODEGEN_OBJ): $(CODEGEN_SRC) $(INCLUDE_DIR)/machine.h $(INCLUDE_DIR)/templates.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(MACHINE_OBJ): $(MACHINE_SRC) $(INCLUDE_DIR)/machine.h | $(BUILD_DIR)
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

$(VALIDATE_OBJ): $(VALIDATE_SRC) $(INCLUDE_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile main
$(MAIN_OBJ): $(MAIN_SRC) $(INCLUDE_DIR)/machine.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link compiler
$(COMPILER): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build every example into dist/<name>.html
build-all: $(COMPILER) | $(DIST_DIR)
	@for f in $(EXAMPLES_DIR)/*.machine; do \
		base=$$(basename $$f .machine); \
		echo "Building $(DIST_DIR)/$$base.html"; \
		$(COMPILER) -o $(DIST_DIR)/$$base.html $$f || exit 1; \
	done

# Build a single example (default: hello-world)
EXAMPLE ?= hello-world
build-example: $(COMPILER) | $(DIST_DIR)
	$(COMPILER) -o $(DIST_DIR)/$(EXAMPLE).html $(EXAMPLES_DIR)/$(EXAMPLE).machine
	@echo "Open $(DIST_DIR)/$(EXAMPLE).html in a browser"

# Compile every example, discarding the output
test-build: $(COMPILER)
	@for f in $(EXAMPLES_DIR)/*.machine; do \
		$(COMPILER) $$f > /dev/null && echo "OK   $$f" || { echo "FAIL $$f"; exit 1; }; \
	done

test: test-build

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(DIST_DIR)

# Install to system (optional)
install: $(COMPILER)
	cp $(COMPILER) /usr/local/bin/turing-compiler

help:
	@echo "Targets:"
	@echo "  all           build the compiler (default)"
	@echo "  build-all     compile every example to dist/<name>.html"
	@echo "  build-example compile one example (EXAMPLE=name)"
	@echo "  test          run the checks"
	@echo "  clean         remove build artifacts"
	@echo "  install       copy the compiler to /usr/local/bin"
