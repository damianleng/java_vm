# JVM — OS Course Project

A Java Virtual Machine implemented from scratch in C. Parses compiled Java `.class` files and executes bytecode directly, without relying on any existing JVM runtime.

Built as an Operating Systems course project to demonstrate core OS concepts: memory management, process execution, resource allocation, and garbage collection.

---

## What It Does

You write Java, compile it with `javac`, and run it with this JVM instead of the standard one. The JVM reads the binary `.class` file, decodes the bytecode instructions, manages its own heap memory, and garbage collects objects that are no longer reachable — all implemented in C.

---

## Scope

### Core Features
- `.class` file parsing (constant pool, fields, methods)
- Bytecode interpreter (arithmetic, control flow, method calls)
- Object creation and field access
- Static and instance method invocation
- Arrays and basic array operations
- Primitive types: int, boolean, byte, char, double
- Garbage collection (mark-and-sweep)
- Basic runtime support (`System.out.println`)

### Extended (Time Permitting)
- Interface support and polymorphism
- Exception handling (`try`/`catch`/`finally`)
- String and basic String methods
- Basic multithreading
- Generational garbage collection

---

## OS Concepts Demonstrated

| Concept | Implementation |
|---|---|
| Memory Management | Custom heap allocator, fragmentation handling, GC |
| Process & Execution Model | Program counter, call stack, method frames |
| Resource Allocation | Finite heap space, object lifetime tracking |
| Concurrency | Thread management, synchronization (extended scope) |

---

## Project Structure

```
.
├── include/          # Header files
│   ├── classfile.h   # Class file structures
│   ├── interpreter.h # Execution frame and opcode definitions
│   ├── heap.h        # Heap allocator interface
│   └── gc.h          # Garbage collector interface
├── src/
│   ├── main.c                    # Entry point
│   ├── classfile/classfile.c     # .class file parser
│   ├── interpreter/interpreter.c # Bytecode execution engine
│   ├── memory/heap.c             # Heap memory management
│   ├── gc/gc.c                   # Mark-and-sweep GC
│   └── runtime/runtime.c         # Native method stubs
├── tests/
│   ├── Hello.java        # Basic println test
│   ├── Arithmetic.java   # Integer arithmetic test
│   ├── Loops.java        # For/while loop test
│   ├── Objects.java      # Object creation and field access test
│   ├── Arrays.java       # Array allocation and access test
│   ├── classes/          # Compiled .class files (generated)
│   └── run_tests.sh      # Test runner script
├── Makefile
├── Dockerfile
└── docker-compose.yml
```

---

## Running with Docker (Recommended)

No compiler or JDK installation needed — just Docker.

**1. Clone the repo and build the image:**
```bash
git clone <repo-url>
cd project
docker compose build
```

**2. Run the full test suite:**
```bash
docker compose run test
```

Expected output:
```
Compiling test files...

PASS  Hello
PASS  Arithmetic
PASS  Loops
PASS  Objects
PASS  Arrays

Results: 5 passed, 0 failed
```

**3. Run a single class file:**
```bash
# The test service compiles .class files into tests/classes/ on your host.
# Run any of them individually:
docker compose run jvm tests/classes/Hello.class
docker compose run jvm tests/classes/Objects.class
```

**Rebuilding after source changes:**
```bash
docker compose build
docker compose run test
```

---

## Running Locally (Optional)

Requires `gcc`, `make`, and a JDK (`javac`).

**macOS:**
```bash
brew install openjdk
export PATH="/opt/homebrew/opt/openjdk/bin:$PATH"
```

**Build and test:**
```bash
make
./tests/run_tests.sh
```

**Run a single class:**
```bash
javac -d tests/classes tests/Hello.java
./jvm tests/classes/Hello.class
```

---

## Language

C (C11), compiled with gcc.
