
# Orus Language Roadmap

This document consolidates the development roadmaps for the Orus language, tracking progress and version increments across multiple development streams.

**Current Version: 0.5.4**

## Version History

- **0.1.0**: Initial release with basic language features  
- **0.5.0**: Current version with generics, modules, error handling, and garbage collection  
- **0.5.1**: Added `input()` builtin for basic user input
- **0.5.2**: Improved diagnostics for repeated module imports
- **0.5.3**: Introduced `std/math` library with core math utilities
- **0.5.4**: Added `const` declarations, embedded standard library and enhanced casting rules

## Completed Major Features

| Feature                | Description                                       | Version Impact     |
|------------------------|---------------------------------------------------|--------------------|
| ✅ Garbage Collection   | Mark-sweep GC replacing manual memory management | 0.3.0 → 0.4.0      |
| ✅ Generic Types (Basic)| Generic functions and structs with type parameters| 0.2.0 → 0.3.0      |
| ✅ Module System        | Modules and `use` statements                     | 0.1.0 → 0.2.0      |
| ✅ Error Handling       | Try/catch blocks for exception handling          | 0.4.0 → 0.5.0      |
| ✅ Dynamic Arrays       | Arrays with push, pop, and len operations        | Minor feature      |
| ✅ User Input           | input() builtin for reading from stdin           | 0.5.0 → 0.5.1      |

---

## Pending Features and Version Targets

### 1. Generics Enhancements

**Status**: Partially implemented (Basic functionality available)

| Task                              | Status           | Priority | Version Impact   |
|-----------------------------------|------------------|----------|------------------|
| Generic Forward Declarations      | Partially done   | High     | 0.5.0 → 0.6.0    |
| Generic Constraints               | Not started      | High     | 0.6.0 → 0.7.0    |
| Improved Type Inference           | Not started      | High     | Minor feature    |
| Generic Arithmetic & Operators    | In progress      | Medium   | Minor feature    |
| Collection and Iterator Support   | Not started      | Medium   | Minor feature    |
| Enhanced Error Reporting          | Not started      | Medium   | Minor feature    |
| Cross-Module Generics             | Not started      | Low      | Minor feature    |
| Generics Documentation            | Not started      | Low      | No impact        |

---

### 3. Memory Management (Garbage Collection)

**Status**: ✅ Fully implemented

| Task                          | Status     | Impact             |
|-------------------------------|------------|--------------------|
| Object Header Structure       | ✅ Complete| Core implementation|
| Value Representation Changes  | ✅ Complete| Core implementation|
| GC State and Integration      | ✅ Complete| Core implementation|
| Mark & Sweep Phases           | ✅ Complete| Core implementation|
| Final Testing & Tuning        | ✅ Complete| Core implementation|

---

### 4. Built-in Functions and Standard Library

**Status**: Initial runtime built-ins completed; standard library modules in planning

The following built-in functions are implemented in the Orus runtime and available in every module:

#### ✅ Core Built-ins (VM-level, implemented)

- `print(values...)` – Formatted console output  
- `len(value)` – Length of a string or array  
- `substring(str, start, len)` – Extract part of a string  
- `push(array, value)` / `pop(array)` – Dynamic array operations  
- `range(start, end)` – Range generator for loops  
- `sum(array)` – Return sum of numeric values in array  
- `min(array)` / `max(array)` – Return min/max value from array  
- `sorted(array, reverse)` – Returns a new sorted array  
- `type_of(value)` / `is_type(value, name)` – Type inspection and checking  
- `input(prompt)` – Console input from user  
- `int(text)` / `float(text)` – Safe type conversion from string  

#### 🧩 Next Built-ins (Planned for runtime inclusion)

- `abs(x)` – Absolute value  
- `round(x)` – Rounding of floats  
- `any(array)` / `all(array)` – Truth aggregation  
- `filter(array, fn)` – Higher-order filtering (requires function-as-value support)  
- `map(array, fn)` – Transformation function (same as above)  

---

### 📦 Standard Library Modules (To be written in Orus)

Orus will include a set of standard library modules located in a `std/` directory. These will be loaded using the `use` statement:

```orus
use std::math
```

| Module         | Description                                     | Target Version   |
|----------------|-------------------------------------------------|------------------|
| `std/math`       | `clamp`, `sqrt`, `average`, constants like `PI` | 0.5.3            |
| `std/random`     | LCG `random`, `randint`, `uniform`, `choice`, `sample`, `shuffle` | 0.6.0            |
| `std/functional` | `map`, `filter`, `reduce`                       | 0.6.0+ (depends on function support) |
| `std/datetime`   | `DateTime` struct, `from_timestamp`, `to_timestamp`, `format`, auto `to_string` | 0.6.0+ |
| `std/os`         | `cwd()`, file I/O, environment info             | 0.6.0+           |
| `std/strings`    | `lowercase`, `trim`, `split`, `replace`         | 0.6.0+           |

---

### Implementation Strategy

- Standard library modules are written in pure Orus and organized in `std/`  
- Only minimal system functions are exposed from the VM runtime (e.g., `_timestamp`)  
- Higher-order functions like `map(fn)` or `filter(fn)` will depend on function-as-value support planned for 0.6.x

---

## Path to Version 1.0

| Milestone | Focus |
|-----------|-------|
| **0.6.0** | Generic forward declarations, stdlib base modules, more built-ins |
| **0.7.0** | Generic constraints |
| **0.8.0** | Expand standard library and core collections |
| **0.9.0** | Concurrency and error handling improvements |
| **1.0.0** | Finalized language stability, documentation, performance |

---

## Development Priorities

### Short-Term (0–3 months)
- Finish generic forward declarations
- Add remaining built-ins: `any`, `all`
- Expand standard library with modules like `functional` and `random`.

### Medium-Term (3–12 months)
- Generic constraints and arithmetic  
- Build out I/O, date/time, strings, and collections  
- Improve diagnostics and error reporting

### Long-Term (12+ months)
- Full concurrency model with async/thread support  
- Language optimizations and bytecode performance  
- Prepare stable 1.0 release

---

## Versioning Strategy

### Semantic Versioning
- **Major** (1.0.0): Stability milestone  
- **Minor** (0.x.0): Features like generics, collections  
- **Patch** (0.0.x): Fixes, minor additions

### Compatibility
- Backward compatibility maintained during 0.x phase  
- Breaking changes clearly documented  
- Migration tools/guides for major version jumps  
