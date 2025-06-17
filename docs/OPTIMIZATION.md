# Orus Optimization Roadmap

This document focuses exclusively on performance and optimization improvements for the Orus programming language runtime and type system.

## 🎯 Goal

Improve the execution performance of the Orus language by leveraging its **static type system** to eliminate unnecessary runtime overhead, avoid type dispatch during interpretation, and generate optimized bytecode or execution paths for primitive operations.

## Performance Optimization via Static Typing

| Task                                          | Status      |
| --------------------------------------------- | ----------- |
| Type-specialized bytecode generation          | ✅ Done |
| Avoid boxing/tagging of known primitive types | ✅ Done |
| Use typed VM stack/registers for primitives   | ✅ Done |
| Lazy typed iterators for `range()`            | ✅ Done |
| Fast-path array push for primitives           | ✅ Done |
| Type-based print & builtins dispatch          | In progress |
| GC-aware execution in tight numeric loops     | Not started |
| Default auto-inference to `i64` over `i32`    | Planned     |
| Overflow-safe numeric promotion               | Planned     |

---

## 🔧 Detailed Strategy

### 1. Type-Specialized Bytecode Generation

* Infer exact types of all expressions during compilation.
* Emit optimized opcodes like `ADD_I64`, `PUSH_I64_CONST`, `PRINT_I64` instead of generic `ADD`, `PUSH`, etc.
* Implemented `OP_I64_CONST` for direct push of 64-bit integer constants.
* Compiler now emits `INC_I64` and `JUMP_IF_LT_I64` for loops with `i64` counters, enabling faster increments and comparisons.
* Type-specialized bytecode generation is now complete, enabling fast numeric loops without extra type checks.

### 2. Avoid Boxing and Tagging for Primitives

* Store raw types like `int64_t`, `float64_t` directly in VM stack/registers.
* Only wrap in a `Value` union when dynamic behavior is necessary.
* Avoid heap allocations for local integer/float values.
* Initial opcodes now skip runtime type checks for integer arithmetic.
* Introduced a parallel `int64_t` stack used by 64-bit integer opcodes.
* Added `vmPushI64` and `vmPopI64` helpers so typed opcodes operate on raw values.
* Removed runtime type checks for all specialized integer operations.
* Phase 2 completed with unboxed primitives across the interpreter.

### 3. Optimize Interpreter Dispatch

* Use a register-based or efficient stack-based VM loop.
* Dispatch opcodes like `ADD_I64`, `SUB_I64`, `EQ_I64` with no dynamic type checking.
* Inline fast-path instruction logic where applicable.
* **Phase 3 completed with optimized dispatch and inlined integer operations.**

### 4. Lazy and Typed `range()` Iterators

* `range(start, end)` now returns a lightweight `RangeIterator` with `int64_t` fields.
* For-loops advance these iterators via the `ITER_NEXT_I64` opcode.
* No arrays are allocated for range iteration.
* Phase complete with the new opcode driving lazy iteration.

### 5. Efficient Arrays and `push()`

* Use contiguous memory for primitive arrays (`int64_t*`, etc.).
* Avoid wrapping values in dynamic containers unless explicitly required.
* Allow `reserve()` for preallocating array capacity.
* Compile `push`, `pop` and `reserve` to specialized opcodes when the array
  type is known at compile time.

### 6. Optimized Print and Built-ins

* Use compile-time dispatch to print functions like `PRINT_I64`, `PRINT_F64`, etc.
* Also apply this to other built-ins like `len`, `type_of`, etc., based on known types.
* Initial typed print opcodes for numeric, boolean and string values are now available.
* Added specialized `LEN_ARRAY` and `LEN_STRING` opcodes for `len()` when the argument type is known.
* Specialized `TYPE_OF_*` opcodes push constant type names when the argument's static type is known.

### 7. GC-Aware Execution

* Avoid tracking of loop-local primitive variables in the GC.
* Disable GC temporarily during tight numeric loops.
* Leverage type info for lifetime tracking.

### 8. Optional: Ahead-of-Time Optimization

* Compile hot blocks or standard library functions to native code.
* Use C or LLVM as backends for critical math-heavy functions.
* Cache optimized bytecode or binaries when possible.

---

## 📦 Example Compilation

```orus
fn main() {
    let mut sum = 0  // inferred as i64
    for i in 0..1_000_000 {
        sum = sum + i
    }
    print(sum)
}
```

Expected Bytecode:

```
PUSH_I64_CONST 0
STORE sum
RANGE_I64 0, 1000000
FOR_LOOP_I64 i
  LOAD sum
  LOAD i
  ADD_I64
  STORE sum
END_LOOP
LOAD sum
PRINT_I64
```

➡️ Runs with **no type checks**, **no heap allocations**, and **no tag switching** at runtime.

---

## 🔚 Final Goal

Enable Orus to **match or exceed Python’s numeric performance** (without JIT) through static typing and optimized execution. This unlocks the next evolution of the VM.

---
