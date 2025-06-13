# Orus Language Roadmap

This document consolidates the development roadmaps for the Orus language, tracking progress and version increments across multiple development streams.

**Current Version: 0.5.3**

## Version History

* **0.1.0**: Initial release with basic language features
* **0.5.0**: Generics, modules, error handling, and garbage collection
* **0.5.1**: Added `input()` builtin for basic user input
* **0.5.2**: Improved diagnostics for repeated module imports
* **0.5.3**: Added `const` declarations for top-level immutable bindings

## ✅ Completed Major Features

| Feature                 | Description                                        | Version Impact |
| ----------------------- | -------------------------------------------------- | -------------- |
| ✅ Garbage Collection    | Mark-sweep GC replacing manual memory management   | 0.3.0 → 0.4.0  |
| ✅ Generic Types (Basic) | Generic functions and structs with type parameters | 0.2.0 → 0.3.0  |
| ✅ Module System         | Modules and `use` statements                       | 0.1.0 → 0.2.0  |
| ✅ Error Handling        | Try/catch blocks for exception handling            | 0.4.0 → 0.5.0  |
| ✅ Dynamic Arrays        | Arrays with push, pop, and len operations          | Minor feature  |
| ✅ User Input            | input() builtin for reading from stdin             | 0.5.0 → 0.5.1  |
| ✅ Core Built-ins        | print, len, push, pop, range, sum, sorted, etc.    | 0.5.0+         |
| ✅ Const Declarations    | Top-level immutable values via `const`             | 0.5.2 → 0.5.3  |

---

## 🔧 Pending Features and Version Targets

### 1. Generic System Enhancements

| Task                            | Status         | Priority | Version Impact |
| ------------------------------- | -------------- | -------- | -------------- |
| Generic Forward Declarations    | Partially done | High     | 0.5.0 → 0.6.0  |
| Generic Constraints             | Not started    | High     | 0.6.0 → 0.7.0  |
| Improved Type Inference         | Not started    | High     | Minor feature  |
| Generic Arithmetic & Operators  | Not started    | Medium   | Minor feature  |
| Collection and Iterator Support | Not started    | Medium   | Minor feature  |
| Enhanced Error Reporting        | Not started    | Medium   | Minor feature  |
| Cross-Module Generics           | Not started    | Low      | Minor feature  |
| Generics Documentation          | Not started    | Low      | No impact      |

---

### 2. Built-in Functions Expansion

| Function     | Status      | Target Version |
| ------------ | ----------- | -------------- |
| `abs(x)`     | Not started | 0.6.0          |
| `round(x)`   | Not started | 0.6.0          |
| `any(array)` | Not started | 0.6.0          |
| `all(array)` | Not started | 0.6.0          |
| `filter(fn)` | Planned     | 0.6.0+         |
| `map(fn)`    | Planned     | 0.6.0+         |

---

### 3. Standard Library Modules (std/)

| Module           | Description                                     | Target Version                       |
| ---------------- | ----------------------------------------------- | ------------------------------------ |
| `std/math`       | `clamp`, `sqrt`, `average`, constants like `PI` | 0.6.0                                |
| `std/random`     | LCG `rand`, `rand_int`, `choice`, `shuffle`     | 0.6.0                                |
| `std/functional` | `map`, `filter`, `reduce`                       | 0.6.0+ (depends on function support) |
| `std/datetime`   | `now()`, `timestamp()`, formatting              | 0.6.0+ (needs runtime `_timestamp`)  |
| `std/os`         | `cwd()`, file I/O, environment info             | 0.6.0+                               |
| `std/strings`    | `lowercase`, `trim`, `split`, `replace`         | 0.6.0+                               |

---

### 4. Public Struct Fields and Methods

| Task                        | Status      | Priority | Target Version |
| --------------------------- | ----------- | -------- | -------------- |
| `pub` on fields and methods | Not started | High     | 0.6.0          |

---

### 5. File I/O and Runtime Utilities

| Task                      | Status      | Target Version |
| ------------------------- | ----------- | -------------- |
| `read_file`, `write_file` | Not started | 0.6.0+         |
| `now_timestamp()`         | Not started | 0.6.0+         |

---

### 6. Assertion and Testing Support

| Task                | Status      | Target Version |
| ------------------- | ----------- | -------------- |
| `assert()` function | Not started | 0.6.0          |
| `orus test` runner  | Not started | 0.6.0+         |

---

### 7. Module System Improvements

| Feature                 | Status      | Target Version |
| ----------------------- | ----------- | -------------- |
| Selective imports       | Not started | 0.6.0+         |
| `pub use` re-exporting  | Not started | 0.6.0+         |
| Submodule folder layout | Not started | 0.6.0+         |

---

## Path to Version 1.0

| Milestone | Focus                                                                                      |
| --------- | ------------------------------------------------------------------------------------------ |
| **0.6.0** | Generic forward declarations, stdlib base modules, visibility improvements, more built-ins |
| **0.7.0** | Generic constraints, struct visibility, file I/O expansion                                 |
| **0.8.0** | Standard library and core collections growth                                               |
| **0.9.0** | Concurrency and error handling improvements                                                |
| **1.0.0** | Final language polish, docs, performance, testing support                                  |

---

## Development Priorities

### Short-Term 

* Finalize generic forward declarations
* Add remaining built-ins: `abs`, `round`, `any`, `all`
* Begin core standard library: `math`, `random`, `functional`

### Medium-Term&#x20;

* Generic constraints and arithmetic
* Build out I/O, date/time, strings, and collections
* Improve diagnostics and error reporting

### Long-Term

* Full concurrency model with async/thread support
* Language optimizations and bytecode performance
* Prepare stable 1.0 release

---

## Versioning Strategy

### Semantic Versioning

* **Major** (1.0.0): Stability milestone
* **Minor** (0.x.0): Features like generics, collections
* **Patch** (0.0.x): Fixes, minor additions

### Compatibility

* Backward compatibility maintained during 0.x phase
* Breaking changes clearly documented
* Migration tools/guides for major version jumps
