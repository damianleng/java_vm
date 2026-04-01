# JVM — OS Course Project

A Java Virtual Machine implemented from scratch in C. Parses compiled Java `.class` files and executes bytecode directly, without relying on any existing JVM runtime.

Built as an Operating Systems course project to demonstrate core OS concepts: memory management, process execution, resource allocation, and garbage collection.

---

## What It Does

You write Java, compile it with `javac`, and run it with this JVM instead of the standard one:

```bash
javac Hello.java
./jvm Hello.class
```

The JVM reads the binary `.class` file, decodes the bytecode instructions, manages its own heap memory, and garbage collects objects that are no longer reachable — all implemented in C.

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
│   ├── interpreter.h # Execution frame
│   ├── heap.h        # Heap allocator interface
│   └── gc.h          # Garbage collector interface
├── src/
│   ├── main.c                  # Entry point
│   ├── classfile/classfile.c   # .class file parser
│   ├── interpreter/interpreter.c # Bytecode execution engine
│   ├── memory/heap.c           # Heap memory management
│   ├── gc/gc.c                 # Mark-and-sweep GC
│   └── runtime/runtime.c       # Native method stubs
├── tests/classes/    # Java .class files for testing
├── Makefile
└── Dockerfile
```

---

## Build & Run

**Requirements:** `gcc`, `make`

```bash
# Build
make

# Run a .class file
./jvm path/to/YourClass.class

# Clean build artifacts
make clean
```

---

## Docker

No gcc? No problem. Run with Docker:

```bash
# Build image
docker build -t myjvm .

# Run a .class file (mount the folder containing your .class files)
docker run --rm -v $(pwd):/classes myjvm /classes/YourClass.class
```

---

## Language

C (C11), compiled with gcc.