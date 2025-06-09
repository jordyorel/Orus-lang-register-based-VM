# Roadmap for Enabling Effective `use` Imports in Orus (C-based Language)

This roadmap outlines the detailed steps to make the `use` import system in Orus functional. 
---

## ✅ Goals

* Support `use math`, `use random::{rand}`, `use datetime as dt`, etc.
* Ensure imported files execute once and expose usable functions, types, and constants.
* Maintain a clean, professional structure aligned with the current codebase.

---

## 🗂️ Where to Implement

| Feature                    | File(s) to Modify                         |
| -------------------------- | ----------------------------------------- |
| Import resolution          | `src/compiler/compiler.c`                 |
| Module file loading        | `src/util/file_utils.c`                   |
| AST representation         | `src/compiler/ast.c`, `include/ast.h`     |
| Parsing new `use` syntax   | `src/parser/parser.c`, `include/parser.h` |
| Symbol definition          | `src/compiler/symtable.c`                 |
| Runtime execution          | `src/vm/vm.c`                             |
| Value handling for modules | `src/vm/value.c`, `include/value.h`       |

---

## 🔧 Function Signature Proposals

### 1. Load Orus file from path

```c
char* load_module_source(const char* resolved_path);
```

### 2. Parse a source string into an AST

```c
ASTNode* parse_module_source(const char* source_code, const char* module_name);
```

### 3. Compile module AST into bytecode

```c
Chunk* compile_module_ast(ASTNode* ast, const char* module_name);
```

### 4. Cache compiled modules globally

```c
typedef struct {
    char* module_name;
    Chunk* bytecode;
    Table exports; // exported functions/types/constants
} Module;

bool register_module(Module*);
Module* get_module(const char* name);
```

---

## 🧩 Step-by-Step Roadmap

### Step 1: Parse `use` Statements ✅

* Extend `parser.c` to recognize:

  ```orus
  use math
  use random::{rand, rand_int}
  use datetime as dt
  use tests::modules::hello_module
  use tests::modules::hello_module as hm
  use tests::modules::hello_module::{greet}
  ```
* Output an `ImportNode` in AST with:

  * full module path (`tests/modules/hello_module.orus`)
  * optional alias name
  * optional list of selected symbols

### Step 2: Resolve Module Path ✅

* In `compiler.c`, convert `use tests::modules::hello_module` to:

  ```c
  tests/modules/hello_module.orus
  ```
* Use the path to locate the module file on disk

### Step 3: Load and Parse Module ✅

* Use `file_utils.c` to load file contents.
* Pass to `parser` → AST → `compiler` → bytecode (like normal program).
* Store result in a global module registry.

### Step 4: Execute Module Once ✅

* Use `get_module()` to check if already loaded.
* If not, compile and run it in an isolated scope.
* Store public functions/values into an `exports` map.

### Step 5: Bind Imports to Symbol Table

* When compiling the main file, inject the imported symbols into its scope.
* If selective import: only bind those.

  ```orus
  use math::{clamp, round}  // Only these are exposed
  use tests::modules::hello_module::{greet}
  ```

### Step 6: Handle Aliasing

* `use datetime as dt` should alias the module in symbol table.
* `use tests::modules::hello_module as hm` binds all public members under `hm`
* Access becomes `hm.greet()`

### Step 7: Prevent Recompilation

* Register compiled modules in a global cache.
* On re-import, retrieve from cache instead of reloading/recompiling.


---

## 🛡️ Error Handling

* Undefined module → "Module `foo` not found"
* Missing export → "Symbol `bar` not found in module `foo`"
* Import cycle → detect and prevent infinite recursion

---

## ✅ Example Flow: `use math::{clamp}`

1. Parser reads and stores use statement
2. Compiler resolves file path
3. File content is read
4. Parsed into AST
5. Compiled to chunk
6. Public symbols are registered in module export table
7. `clamp` is added to the current file’s symbol table
8. Later function calls to `clamp(...)` are resolved

---

## 📌 Enhancements

* Allow `pub` keyword to mark exported functions in modules
* Support `use *` to import everything
* Expose module metadata (name, path)
* CLI flag to trace imports for debugging

---
