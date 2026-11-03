#include "../include/lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
static void generateStateTransitions(ASTNode *node);
static void generateAlphabet(ASTNode *node);
static void generateBlank(ASTNode *node);
static void generateInput(ASTNode *node);
static void generateSimulator(void);
static void collectStatements(ASTNode *node);
static ASTNode *findChild(ASTNode *node, NodeType type);

// Global state for code generation
static char *blankSymbol = "$";
static char **alphabet = NULL;
static int alphabetSize = 0;
static char *initialState = NULL;
static char *inputString = "";
static int stateCount = 0;
static char **stateNames = NULL;
static ASTNode **stateNodes = NULL;
static ASTNode **allStatements = NULL;
static int statementCount = 0;

void generateCode(ASTNode *root, char *startState) {
  // First pass: collect all statements from nested MACHINE nodes
  statementCount = 0;
  allStatements = NULL;
  collectStatements(root);

  // Process all collected statements
  for (int i = 0; i < statementCount; i++) {
    ASTNode *stmt = allStatements[i];
    if (stmt->type == STATEMENT && stmt->n > 0) {
      ASTNode *firstChild = stmt->children[0];
      if (firstChild->type == TERMINAL) {
        if (strcmp(firstChild->value, "BLANK") == 0) {
          generateBlank(stmt);
        } else if (strcmp(firstChild->value, "ALPHABET") == 0) {
          generateAlphabet(stmt);
        } else if (strcmp(firstChild->value, "STATE") == 0) {
          // Count states and store them
          stateCount++;
          stateNames = realloc(stateNames, stateCount * sizeof(char *));
          stateNodes = realloc(stateNodes, stateCount * sizeof(ASTNode *));
          stateNames[stateCount - 1] = stmt->children[1]->value;
          stateNodes[stateCount - 1] = stmt;
        } else if (strcmp(firstChild->value, "INPUT") == 0) {
          generateInput(stmt);
        } else if (strcmp(firstChild->value, "START") == 0) {
          // START statement sets the initial state
          if (stmt->n >= 3) {
            initialState = stmt->children[2]->value;
          }
        }
      }
    }
  }

  // Free temporary statement array
  free(allStatements);
  allStatements = NULL;
  statementCount = 0;

  // If no START statement was found, use the first state as default
  if (initialState == NULL && stateCount > 0) {
    initialState = stateNames[0];
  }

  // Override with startState from parser if provided
  if (startState != NULL) {
    initialState = startState;
  }

  // Generate JavaScript output
  printf("// Turing Machine Simulation\n");
  printf("// Generated from Turing Machine specification\n\n");

  // Configuration object
  printf("const turingMachine = {\n");
  printf("  blank: '%s',\n", blankSymbol);
  printf("  alphabet: [");
  for (int i = 0; i < alphabetSize; i++) {
    printf("'%s'", alphabet[i]);
    if (i < alphabetSize - 1)
      printf(", ");
  }
  printf("],\n");
  printf("  initialState: '%s',\n", initialState);
  printf("  input: '%s',\n", inputString);
  printf("  states: {\n");

  // Generate state transitions
  for (int i = 0; i < stateCount; i++) {
    generateStateTransitions(stateNodes[i]);
    if (i < stateCount - 1)
      printf(",\n");
  }

  printf("\n  }\n");
  printf("};\n\n");

  // Generate the simulator/visualizer
  generateSimulator();
}

// Recursively collect all STATEMENT nodes from nested MACHINE structures
static void collectStatements(ASTNode *node) {
  if (!node)
    return;

  if (node->type == MACHINE) {
    for (int i = 0; i < node->n; i++) {
      collectStatements(node->children[i]);
    }
  } else if (node->type == STATEMENT) {
    allStatements =
        realloc(allStatements, (statementCount + 1) * sizeof(ASTNode *));
    allStatements[statementCount++] = node;
  } else {
    // Recursively check children
    for (int i = 0; i < node->n; i++) {
      collectStatements(node->children[i]);
    }
  }
}

static void generateBlank(ASTNode *stmt) {
  if (stmt->n >= 3) {
    blankSymbol = stmt->children[2]->value;
  }
}

static void generateAlphabet(ASTNode *stmt) {
  ASTNode *symlist = findChild(stmt, SYMLIST);
  if (!symlist)
    return;

  ASTNode *symbols = findChild(symlist, SYMBOLS);
  if (!symbols)
    return;

  // First, count symbols by traversing the left-recursive chain
  int count = 0;
  ASTNode *curr = symbols;
  while (curr && curr->type == SYMBOLS) {
    count++;
    if (curr->n == 1)
      break;
    curr = curr->children[0];
  }

  alphabet = malloc(count * sizeof(char *));
  alphabetSize = 0;

  // Collect symbols in reverse order (because of left recursion)
  char **temp = malloc(count * sizeof(char *));
  int tempCount = 0;

  curr = symbols;
  while (curr && curr->type == SYMBOLS) {
    if (curr->n == 1) {
      temp[tempCount++] = curr->children[0]->value;
    } else if (curr->n == 3) {
      temp[tempCount++] = curr->children[2]->value;
    }
    if (curr->n == 1)
      break;
    curr = curr->children[0];
  }

  // Reverse to get correct order
  for (int i = tempCount - 1; i >= 0; i--) {
    alphabet[alphabetSize++] = temp[i];
  }
  free(temp);
}

static void generateInput(ASTNode *stmt) {
  ASTNode *strNode = findChild(stmt, STRING);
  if (!strNode)
    return;

  // Handle empty input (n=0)
  if (strNode->n == 0) {
    inputString = strdup("");
    return;
  }

  // Count symbols first
  int count = 0;
  ASTNode *curr = strNode;
  while (curr && curr->type == STRING) {
    count++;
    if (curr->n == 1)
      break;
    curr = curr->children[0];
  }

  // Collect in reverse order
  char **temp = malloc(count * sizeof(char *));
  int tempCount = 0;

  curr = strNode;
  while (curr && curr->type == STRING) {
    if (curr->n == 1) {
      temp[tempCount++] = curr->children[0]->value;
    } else if (curr->n == 2) {
      temp[tempCount++] = curr->children[1]->value;
    }
    if (curr->n == 1)
      break;
    curr = curr->children[0];
  }

  // Build string in correct order
  char buffer[1024] = "";
  for (int i = tempCount - 1; i >= 0; i--) {
    strcat(buffer, temp[i]);
  }
  free(temp);
  inputString = strdup(buffer);
}

static void generateStateTransitions(ASTNode *stmt) {
  char *stateName = stmt->children[1]->value;
  ASTNode *blk = findChild(stmt, BLK);
  if (!blk)
    return;

  ASTNode *transitions = findChild(blk, TRANSITIONS);
  if (!transitions)
    return;

  printf("    '%s': {\n", stateName);

  // First, collect all transitions in a list (since they're left-recursive, we
  // get them in reverse)
  int transCount = 0;
  ASTNode **transList = NULL;
  ASTNode *curr = transitions;
  while (curr && curr->type == TRANSITIONS) {
    transList = realloc(transList, (transCount + 1) * sizeof(ASTNode *));
    transList[transCount++] = curr;
    if (curr->n == 6)
      break;                  // Single transition (base case)
    curr = curr->children[0]; // Move to previous transition in chain
  }

  // Process transitions in correct order (reverse of collection)
  int first = 1;
  for (int i = transCount - 1; i >= 0; i--) {
    curr = transList[i];
    char *symbol = NULL;
    char *write = NULL;
    char *next = NULL;
    char *move = NULL;

    int symbolIndex, actionsIndex;
    if (curr->n == 6) {
      // Single transition: SYMBOL ASSIGN LBRACE ACTIONS RBRACE ENDL
      symbolIndex = 0;
      actionsIndex = 3;
    } else {
      // Chained: TRANSITIONS SYMBOL ASSIGN LBRACE ACTIONS RBRACE ENDL
      symbolIndex = 1;
      actionsIndex = 4;
    }

    if (curr->n > symbolIndex &&
        curr->children[symbolIndex]->type == TERMINAL) {
      symbol = curr->children[symbolIndex]->value;
    }

    ASTNode *actions = curr->children[actionsIndex];
    if (actions && actions->type == ACTIONS) {
      // Traverse the left-recursive ACTIONS chain
      ASTNode *actionChain = actions;
      // First, collect all actions
      int actionCount = 0;
      ASTNode **actionList = NULL;
      ASTNode *ac = actionChain;
      while (ac && ac->type == ACTIONS) {
        actionList = realloc(actionList, (actionCount + 1) * sizeof(ASTNode *));
        actionList[actionCount++] =
            ac->children[ac->n - 1]; // Last child is the ACTION
        if (ac->n == 1)
          break;
        ac = ac->children[0];
      }
      // Process actions
      for (int j = 0; j < actionCount; j++) {
        ASTNode *a = actionList[j];
        if (a->type == ACTION && a->n == 3) {
          char *actionType = a->children[0]->value;
          char *actionValue = a->children[2]->value;
          if (strcmp(actionType, "WRITE") == 0)
            write = actionValue;
          else if (strcmp(actionType, "NEXT") == 0)
            next = actionValue;
          else if (strcmp(actionType, "MOVE") == 0)
            move = actionValue;
        }
      }
      free(actionList);
    }

    if (symbol) {
      if (!first)
        printf(",\n");
      first = 0;
      printf("      '%s': { write: '%s', next: '%s', move: '%s' }", symbol,
             write ? write : blankSymbol, next ? next : "HALT",
             move ? move : "RIGHT");
    }
  }
  free(transList);

  printf("\n    }");
}

static void generateSimulator() {
  printf("// Turing Machine Visualizer - barebones working version\n");
  printf("class TuringVisualizer {\n");
  printf("  constructor(machine, containerId = 'turing-viz') {\n");
  printf("    this.machine = machine;\n");
  printf("    this.container = document.getElementById(containerId);\n");
  printf("    this.tape = [];\n");
  printf("    this.head = 0;\n");
  printf("    this.state = machine.initialState;\n");
  printf("    this.stepCount = 0;\n");
  printf("    this.running = false;\n");
  printf("    this.speed = 500;\n");
  printf("    this.history = [];\n");
  printf("    this.cellElements = [];\n");
  printf("    this.timerId = null;\n");
  printf("    this.init();\n");
  printf("  }\n\n");
  printf("  init() {\n");
  printf("    console.log('Initializing machine:', this.machine.initialState, this.machine.input);\n");
  printf("    const inputChars = this.machine.input.split('');\n");
  printf("    const blank = this.machine.blank;\n");
  printf("    const PAD = 30;\n");
  printf("    this.tape = [];\n");
  printf("    for (let i = 0; i < PAD; i++) this.tape.push(blank);\n");
  printf("    for (const c of inputChars) this.tape.push(c);\n");
  printf("    for (let i = 0; i < PAD; i++) this.tape.push(blank);\n");
  printf("    this.head = PAD;\n");
  printf("    this.stepCount = 0;\n");
  printf("    this.history = [];\n");
  printf("    this.render();\n");
  printf("  }\n\n");
  printf("  step() {\n");
  printf("    if (this.running) this.stop(); // Stop auto-run if stepping manually\n");
  printf("    if (this.state === 'HALT') {\n");
  printf("      console.log('Already halted');\n");
  printf("      return false;\n");
  printf("    }\n");
  printf("    const currentSymbol = this.tape[this.head] || this.machine.blank;\n");
  printf("    const stateDef = this.machine.states[this.state];\n");
  printf("    if (!stateDef || !stateDef[currentSymbol]) {\n");
  printf("      console.log('No transition for state', this.state, 'symbol', currentSymbol);\n");
  printf("      return false;\n");
  printf("    }\n");
  printf("    const transition = stateDef[currentSymbol];\n");
  printf("    console.log('Step', this.stepCount, ': state=', this.state, 'read=', currentSymbol, '-> write=', transition.write, 'next=', transition.next, 'move=', transition.move);\n");
  printf("    this.history.push({ tape: [...this.tape], head: this.head, state: this.state, step: this.stepCount });\n");
  printf("    this.tape[this.head] = transition.write;\n");
  printf("    this.state = transition.next;\n");
  printf("    if (transition.move === 'LEFT') {\n");
  printf("      this.head--;\n");
  printf("      if (this.head < 0) {\n");
  printf("        this.tape.unshift(this.machine.blank);\n");
  printf("        this.head = 0;\n");
  printf("      }\n");
  printf("    } else {\n");
  printf("      this.head++;\n");
  printf("      if (this.head >= this.tape.length) {\n");
  printf("        this.tape.push(this.machine.blank);\n");
  printf("      }\n");
  printf("    }\n");
  printf("    this.stepCount++;\n");
  printf("    this.render();\n");
  printf("    return this.state !== 'HALT';\n");
  printf("  }\n\n");
  printf("  run() {\n");
  printf("    if (this.running) return;\n");
  printf("    this.stop(); // Clear any existing timer\n");
  printf("    this.running = true;\n");
  printf("    const doStep = () => {\n");
  printf("      if (!this.running || this.state === 'HALT') {\n");
  printf("        this.running = false;\n");
  printf("        this.timerId = null;\n");
  printf("        this.updateControls();\n");
  printf("        return;\n");
  printf("      }\n");
  printf("      this.step();\n");
  printf("      if (this.running) {\n");
  printf("        this.timerId = setTimeout(doStep, this.speed);\n");
  printf("      }\n");
  printf("    }\n");
  printf("    doStep();\n");
  printf("    this.updateControls();\n");
  printf("  }\n\n");
  printf("  stop() {\n");
  printf("    this.running = false;\n");
  printf("    if (this.timerId) {\n");
  printf("      clearTimeout(this.timerId);\n");
  printf("      this.timerId = null;\n");
  printf("    }\n");
  printf("    this.updateControls();\n");
  printf("  }\n\n");
  printf("  reset() {\n");
  printf("    this.stop();\n");
  printf("    this.init();\n");
  printf("  }\n\n");
  printf("  undo() {\n");
  printf("    if (this.history.length === 0) return;\n");
  printf("    const prev = this.history.pop();\n");
  printf("    this.tape = prev.tape;\n");
  printf("    this.head = prev.head;\n");
  printf("    this.state = prev.state;\n");
  printf("    this.stepCount = prev.step;\n");
  printf("    this.render();\n");
  printf("  }\n\n");
  printf("  setSpeed(ms) { this.speed = parseInt(ms, 10); }\n\n");
  printf("  render() {\n");
  printf("    if (!this.container) return;\n");
  printf("    const VISIBLE = 41;\n");
  printf("    const half = Math.floor(VISIBLE / 2);\n");
  printf("    let start = this.head - half;\n");
  printf("    if (start < 0) start = 0;\n");
  printf("    if (start + VISIBLE > this.tape.length) start = this.tape.length - VISIBLE;\n");
  printf("    if (start < 0) start = 0;\n");
  printf("    const end = Math.min(start + VISIBLE, this.tape.length);\n");
  printf("    \n");
  printf("    const tapeDiv = this.container.querySelector('.tape');\n");
  printf("    if (tapeDiv) {\n");
  printf("      tapeDiv.innerHTML = '';\n");
  printf("    } else {\n");
  printf("      this.container.innerHTML = '';\n");
  printf("      this.buildUI();\n");
  printf("      return;\n");
  printf("    }\n");
  printf("    this.cellElements = [];\n");
  printf("    for (let i = start; i < end; i++) {\n");
  printf("      const cell = document.createElement('div');\n");
  printf("      cell.className = 'cell';\n");
  printf("      cell.dataset.index = i;\n");
  printf("      if (i === this.head) cell.classList.add('head');\n");
  printf("      if (i >= 30 && i < 30 + this.machine.input.length && this.stepCount === 0) {\n");
  printf("        cell.classList.add('input');\n");
  printf("      }\n");
  printf("      const sym = document.createElement('div');\n");
  printf("      sym.className = 'symbol';\n");
  printf("      sym.textContent = this.tape[i] || this.machine.blank;\n");
  printf("      if (i === this.head) {\n");
  printf("        const marker = document.createElement('div');\n");
  printf("        marker.className = 'head-marker';\n");
  printf("        marker.textContent = '\\u25BC';\n");
  printf("        cell.appendChild(marker);\n");
  printf("      }\n");
  printf("      cell.appendChild(sym);\n");
  printf("      tapeDiv.appendChild(cell);\n");
  printf("      this.cellElements.push(cell);\n");
  printf("    }\n");
  printf("    this.updateStateDisplay();\n");
  printf("    this.updateControls();\n");
  printf("  }\n\n");
  printf("  buildUI() {\n");
  printf("    this.container.innerHTML = '';\n");
  printf("    // Top bar with state and controls\n");
  printf("    const topBar = document.createElement('div');\n");
  printf("    topBar.className = 'top-bar';\n");
  printf("    this.stateDiv = document.createElement('div');\n");
  printf("    this.stateDiv.className = 'state-display';\n");
  printf("    topBar.appendChild(this.stateDiv);\n");
  printf("    const controls = document.createElement('div');\n");
  printf("    controls.className = 'controls';\n");
  printf("    controls.innerHTML = ''\n");
  printf("      + '<button id=\"btn-step\">Step</button>'\n");
  printf("      + '<button id=\"btn-run\">Run</button>'\n");
  printf("      + '<button id=\"btn-stop\">Stop</button>'\n");
  printf("      + '<button id=\"btn-undo\">Undo</button>'\n");
  printf("      + '<button id=\"btn-reset\">Reset</button>'\n");
  printf("      + '<label>Speed: <input type=\"range\" id=\"speed\" min=\"50\" max=\"2000\" value=\"500\"></label>'\n");
  printf("    topBar.appendChild(controls);\n");
  printf("    this.container.appendChild(topBar);\n");
  printf("    // Tape area\n");
  printf("    const tapeWrap = document.createElement('div');\n");
  printf("    tapeWrap.className = 'tape-wrapper';\n");
  printf("    const tapeDiv = document.createElement('div');\n");
  printf("    tapeDiv.className = 'tape';\n");
  printf("    tapeWrap.appendChild(tapeDiv);\n");
  printf("    this.container.appendChild(tapeWrap);\n");
  printf("    // Bind events\n");
  printf("    document.getElementById('btn-step').onclick = () => this.step();\n");
  printf("    document.getElementById('btn-run').onclick = () => this.run();\n");
  printf("    document.getElementById('btn-stop').onclick = () => this.stop();\n");
  printf("    document.getElementById('btn-undo').onclick = () => this.undo();\n");
  printf("    document.getElementById('btn-reset').onclick = () => this.reset();\n");
  printf("    document.getElementById('speed').oninput = (e) => this.setSpeed(e.target.value);\n");
  printf("    this.render();\n");
  printf("  }\n\n");
  printf("  updateStateDisplay() {\n");
  printf("    if (!this.stateDiv) return;\n");
  printf("    this.stateDiv.textContent = 'State: ' + this.state + ' | Step: ' + this.stepCount;\n");
  printf("  }\n\n");
  printf("  updateControls() {\n");
  printf("    const halted = this.state === 'HALT';\n");
  printf("    document.getElementById('btn-step').disabled = halted || this.running;\n");
  printf("    document.getElementById('btn-run').disabled = halted || this.running;\n");
  printf("    document.getElementById('btn-stop').disabled = !this.running;\n");
  printf("    document.getElementById('btn-undo').disabled = this.history.length === 0;\n");
  printf("  }\n");
  printf("}\n\n");// Initialize visualizer when DOM is ready\n");
  printf("let viz;\n");
  printf("function initViz() { viz = new TuringVisualizer(turingMachine); }\n");
  printf("if (document.readyState === 'loading') {\n");
  printf("  document.addEventListener('DOMContentLoaded', initViz);\n");
  printf("} else {\n");
  printf("  initViz();\n");
  printf("}\n");
  printf("\n");
  printf("// Export for module usage\n");
  printf("if (typeof module !== 'undefined' && module.exports) {\n");
  printf("  module.exports = { turingMachine, TuringVisualizer };\n");
  printf("}\n");
}


static ASTNode *findChild(ASTNode *node, NodeType type) {
  if (!node)
    return NULL;
  for (int i = 0; i < node->n; i++) {
    if (node->children[i]->type == type) {
      return node->children[i];
    }
  }
  return NULL;
}
