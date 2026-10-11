# MiniC Compiler

A **multi-stage compiler** for a subset of C, targeting **x86-64 assembly** (AT&T syntax), written in **C++17**.

Built from scratch with zero parser-generator dependencies — hand-written recursive-descent parser, 3-address IR with CFG-based optimizations, linear-scan register allocation with spilling, GDB-compatible DWARF debug info, and a symbol-table-driven linker helper for multi-file compilation.

## Architecture
<img width="1502" height="476" alt="image" src="https://github.com/user-attachments/assets/5fb32269-08e1-4e97-861d-438913528a9f" />
<img width="1500" height="677" alt="image" src="https://github.com/user-attachments/assets/b3228e01-8d01-401f-964e-bebe2c607d62" />

```
┌─────────────────────────────────────────────────────────────────┐
│                        MiniC Compiler Pipeline                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────┐    ┌────────┐    ┌─────┐    ┌──────────────────┐   │
│  │ Source   │───▶│ Lexer  │───▶│Tokens│───▶│ Parser           |   
│  │ (.mc)   │    │        │    │     │    │ (Recursive       │   │
│  └─────────┘    └────────┘    └─────┘    │  Descent + Pratt)│   │
│                                           └────────┬─────────┘  │
│                                                    │            │
│                                                    ▼            │
│                                              ┌──────────┐       │
│                                              │   AST    │       │
│                                              └────┬─────┘       │
│                                                   │             │
│                                                   ▼             │
│                                          ┌────────────────┐     │
│                                          │ Semantic       │     │
│                                          │ Analysis       │     │
│                                          │ (Type Check,   │     │
│                                          │  Scope Resolve)│     │
│                                          └───────┬────────┘     │
│                                                  │              │
│                                                  ▼              │
│                                         ┌─────────────────┐     │
│                                         │ IR Generation   │     │
│                                         │ (AST → TAC)     │     │
│                                         └───────┬─────────┘     │
│                                                 │               │
│                                                 ▼               │
│                                        ┌──────────────────┐     │
│                                        │ CFG Construction │     │
│                                        │ (Basic Blocks)   │     │
│                                        └───────┬──────────┘     │
│                                                │                │
│                                                ▼                │
│                                   ┌────────────────────────┐    │
│                                   │    Optimization Passes  │   │
│                                   │  ┌──────────────────┐  │    │
│                                   │  │ Constant Folding │  │    │
│                                   │  ├──────────────────┤  │    │
│                                   │  │ Copy Propagation │  │    │
│                                   │  ├──────────────────┤  │    │
│                                   │  │ Dead Code Elim.  │  │    │
│                                   │  └──────────────────┘  │    │
│                                   └───────────┬────────────┘    │
│                                               │                 │
│                              ┌────────────────┼────────────────┐│
│                              ▼                ▼                ▼│
│                    ┌──────────────┐  ┌──────────────┐  ┌───────┐│
│                    │ Register     │  │ DWARF Debug  │  │Linker ││
│                    │ Allocator    │  │ Info Emitter │  │Helper ││
│                    │ (Linear Scan │  │ (.debug_info │  │(Multi ││
│                    │  + Spilling) │  │  .debug_line)│  │ TU)   ││
│                    └──────┬───────┘  └──────┬───────┘  └───┬───┘│
│                           │                 │              │    │
│                           ▼                 │              │    │
│                    ┌──────────────┐         │              │    │
│                    │ x86-64 Code │◀────────┘              │     |
│                    │ Emitter     │                         │    │
│                    │ (AT&T .s)   │◀────────────────────────┘    │
│                    └──────┬───────┘                              │
│                           │                                      │
│                           ▼                                      │
│                    ┌──────────────┐                              │
│                    │ Assembly     │                              │
│                    │ Output (.s)  │                              │
│                    └──────────────┘                              │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## Features

### Stage 1 — Lexer & Parser
- **Hand-written recursive-descent parser** (no ANTLR/Bison/Flex)
- **Pratt parsing** for operator precedence (clean, extensible expression handling)
- Full **AST construction** with 20+ node types
- Supports: variable declarations, assignments, arithmetic, conditionals (`if`/`else`), loops (`while`/`for`), functions with parameters and return values, pointer arithmetic, arrays

### Stage 2 — Semantic Analysis
- **Type checking** (`int`, `char`, pointer types of arbitrary depth)
- **Scope resolution** using a stack-of-maps symbol table (nested scopes, block-level scoping)
- Errors for: undeclared variables, type mismatches, redeclarations, invalid lvalue assignments, argument count mismatches

### Stage 3 — IR Generation
- Lowers AST to **3-address code (TAC)** intermediate representation
- Builds a **Control Flow Graph (CFG)** with basic blocks
- Each basic block tracks predecessors and successors
- Short-circuit evaluation for `&&` and `||`
- Pointer arithmetic with automatic scaling

### Stage 4 — Optimizations
- **Constant folding** — evaluate compile-time constant expressions (e.g., `2 + 3 → 5`)
- **Dead code elimination** — remove unreachable blocks and unused assignments via liveness analysis
- **Copy propagation** — replace `y = x; z = y` with `z = x`
- Fixed-point iteration until no more changes
- Target: ~25% instruction reduction

### Stage 5 — x86-64 Code Generation
- Emits **AT&T syntax x86-64 assembly** (`.s` files)
- **Linear-scan register allocation** with spilling to stack when registers are exhausted
- Proper **function prologues/epilogues** (`pushq %rbp`, `movq %rsp, %rbp`, etc.)
- System V AMD64 ABI calling convention (args in `%rdi`, `%rsi`, `%rdx`, ...)
- Pointer arithmetic and dereferencing in emitted assembly
- 16-byte stack alignment enforcement

### Stage 6 — Debug Info & Linker Helper
- **GDB-compatible DWARF debug info** emission:
  - `.file` and `.loc` directives for source-line mapping
  - `.debug_info` section with `DW_TAG_compile_unit` and `DW_TAG_subprogram` DIEs
  - `.debug_abbrev` section with proper abbreviation tables
  - Enables GDB `list`, `break`, `step`, `info functions`, `backtrace`
- **Symbol-table-driven linker helper**:
  - Resolves external labels across multiple translation units
  - Detects duplicate definitions and unresolved symbols
  - Generates link maps showing symbol resolution
  - Mirrors PTXAS front-end responsibilities

## Project Structure

```
MiniC_Compiler/
├── CMakeLists.txt              # CMake build system
├── README.md                   # This file
├── docs/
│   └── grammar.md              # Formal EBNF grammar
├── src/
│   ├── main.cpp                # Driver (CLI, pipeline orchestration)
│   ├── common/                 # Shared utilities
│   │   ├── error.h/cpp         # Diagnostic engine
│   │   └── source_location.h   # Source location tracking
│   ├── lexer/                  # Stage 1a: Tokenization
│   │   ├── token.h             # Token types
│   │   └── lexer.h/cpp         # Hand-written scanner
│   ├── parser/                 # Stage 1b: Parsing
│   │   ├── ast.h               # AST node hierarchy (20+ types)
│   │   ├── ast_printer.h/cpp   # AST pretty-printer
│   │   └── parser.h/cpp        # Recursive-descent + Pratt parser
│   ├── sema/                   # Stage 2: Semantic Analysis
│   │   ├── type.h              # Type system (int, char, void, pointers)
│   │   ├── symbol_table.h/cpp  # Scoped symbol table
│   │   └── sema.h/cpp          # Type checker & scope resolver
│   ├── ir/                     # Stage 3: IR Generation
│   │   ├── tac.h/cpp           # Three-address code instructions
│   │   ├── ir_gen.h/cpp        # AST → TAC lowering
│   │   ├── cfg.h/cpp           # Basic blocks & CFG construction
│   │   └── ir_printer.h/cpp    # IR textual dump
│   ├── opt/                    # Stage 4: Optimizations
│   │   ├── pass.h              # Abstract optimization pass
│   │   ├── constant_folding.h/cpp
│   │   ├── dead_code_elim.h/cpp
│   │   └── copy_propagation.h/cpp
│   ├── codegen/                # Stage 5: Code Generation
│   │   ├── reg_alloc.h/cpp     # Linear-scan register allocator
│   │   ├── x86_emitter.h/cpp   # AT&T x86-64 assembly emitter
│   │   └── codegen.h/cpp       # Codegen orchestrator
│   ├── debug/                  # Stage 6a: Debug Info
│   │   ├── source_map.h/cpp    # Assembly ↔ source line mapping
│   │   └── dwarf_emitter.h/cpp # DWARF stub generation
│   └── linker/                 # Stage 6b: Linker Helper
│       ├── symbol_resolver.h/cpp # Cross-TU symbol resolution
│       └── link_helper.h/cpp   # Multi-file compilation driver
└── tests/
    ├── conftest.py             # pytest fixtures
    ├── test_e2e.py             # End-to-end tests
    ├── test_extended.py        # Extended test coverage
    └── inputs/                 # 100+ test .mc files
        ├── basics/             # Arithmetic, operators, literals
        ├── control/            # if/else, while, for, nesting
        ├── functions/          # Calls, recursion, multi-function
        ├── pointers/           # Dereference, address-of, double pointers
        ├── scopes/             # Nested scopes, shadowing
        ├── optimizations/      # Constant fold, copy prop, DCE
        ├── register_spill/     # >13 live vars, deep expressions
        └── multi_tu/           # Cross-file function calls
```

## Building

### Prerequisites
- **C++17 compiler** (GCC 7+ or Clang 5+)
- **CMake** 3.16+
- **GNU Assembler** (`as`) and **linker** (`ld`) — available on Linux/WSL
- **Python 3** + **pytest** (for test harness)
- **Valgrind** (optional, for memory checking)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/manaskng/minic-compiler.git
cd minic-compiler

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# The compiler binary is at: build/minic
```

## Usage

### Single File Compilation
```bash
# Compile a .mc file to assembly
./build/minic examples/hello.mc -o hello.s

# Assemble and link
gcc -no-pie -g hello.s -o hello

# Run
./hello
```

### Multi-File Compilation (Linker Helper)
```bash
# Compile and link multiple translation units
./build/minic main.mc --link math.mc utils.mc -o program --link-map
```

### Debug Flags
```bash
./build/minic input.mc -o out.s --dump-ast         # Print AST
./build/minic input.mc -o out.s --dump-ir-before    # Print IR before optimization
./build/minic input.mc -o out.s --dump-ir-after     # Print IR after optimization
./build/minic input.mc -o out.s --dump-cfg          # Print CFG
./build/minic input.mc -o out.s --no-opt            # Disable optimizations
./build/minic input.mc -o out.s -v                  # Verbose (show opt stats)
```

### Debugging with GDB
```bash
# Compile with DWARF debug info (automatic)
./build/minic input.mc -o input.s
gcc -no-pie -g input.s -o input

# Debug with GDB
gdb ./input
(gdb) list              # Show source lines
(gdb) break input.mc:5  # Set breakpoint at source line
(gdb) run               # Run program
(gdb) step              # Step through source
(gdb) info functions    # List all functions
(gdb) backtrace         # Show call stack
```

## Running Tests

```bash
# Install pytest
pip install pytest

# Run all tests
cd tests
pytest -v

# Run specific test category
pytest test_e2e.py::TestBasics -v
pytest test_e2e.py::TestPointers -v
pytest test_e2e.py::TestOptimizations -v
```

### Valgrind Memory Check
```bash
cd build
make valgrind
```

## Example

### Input (`hello.mc`)
```c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    printf("%d\n", factorial(5));
    return 0;
}
```

### Compilation
```bash
./build/minic hello.mc -o hello.s -v
# Output: Optimization: 45 → 34 instructions (24.4% reduction)
```

### Generated Assembly (excerpt)
```asm
    .file 1 "hello.mc"
    .text
    .globl  factorial
    .type   factorial, @function
factorial:
    pushq   %rbp
    movq    %rsp, %rbp
    subq    $128, %rsp
    movq    %rdi, -8(%rbp)          # param: n
    .loc 1 2 0
    movq    -8(%rbp), %rax          # load n
    movq    $1, %rcx
    cmpq    %rcx, %rax              # n <= 1?
    setle   %al
    movzbq  %al, %rax
    cmpq    $0, %rax
    je      .L1
    .loc 1 2 0
    movq    $1, %rax                # return 1
    jmp     .Lfactorial_epilogue
.L1:
    .loc 1 3 0
    ...                             # n * factorial(n - 1)
```

## Tech Stack
- **C++17** — core compiler implementation
- **x86-64 Assembly** — target architecture (AT&T syntax, System V ABI)
- **DWARF** — debug info format (version 4)
- **CMake** — build system
- **pytest** — test harness (100+ test cases)
- **Valgrind** — memory leak detection

