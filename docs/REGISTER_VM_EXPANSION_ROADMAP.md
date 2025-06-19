# Register VM Feature Expansion Roadmap

This document tracks capabilities still missing from the register-based virtual machine and outlines a phased plan to implement them.

## 🧩 Updated Missing Features Checklist

| Limitation | Can Be Optimized? | Notes |
|-----------|-------------------|------|
| ✅ i64 integer arithmetic | ✅ Done | Already implemented (ADD_I64, etc.) |
| ❌ f64 floating-point support | ✅ Planned | Needs: ADD_F64, MUL_F64, type-aware registers or union |
| ❌ Function call stack & return values | ✅ Add call frames | No CALL, RET, or function context yet |
| ❌ Arrays and indexing | ✅ Phase 2 | No support for arrays or push/get/set |
| ❌ Pointer or memory access ops | ✅ Phase 2 | No memory-style register dereferencing or heap refs |
| ❌ Register allocator | ✅ Optional (Phase 3) | Helps reduce register waste or improve spill handling |
| ❌ Dynamic type support (e.g., strings) | ✅ Optional (Phase 3) | Current VM is strictly int64_t; no support for string, bool, null |
| ❌ GC integration | ✅ Later phase | Current VM ignores heap allocation and GC roots |
| ❌ Structs / field access | ✅ Optional (Phase 3) | Needed for user-defined types (user.name, etc.) |
| ❌ Constants pool / global values | ✅ Phase 2 | No opcode support for loading named constants (LOAD_CONST) |
| ❌ Comparison and branching | ✅ Immediate | No EQ, LT, GT, JUMP_IF, etc. yet |
| ❌ Stack-based fallback or hybrid mode | ✅ Optional | Could support mixed stack/register execution for legacy compatibility |
| ❌ Error handling / traps | ✅ Add support | No VM-level error recovery or panics |
| ❌ Debugging hooks / tracing | ✅ Nice to have | Could add TRACE, line numbers, or source tracking |
| ❌ Opcode metadata or assembler tools | ✅ External tooling | Not essential now, but will matter for maintainability |

## 🛣️ Roadmap

### Phase 1 – Core VM Capabilities
- Implement `f64` arithmetic instructions (`ADD_F64`, `SUB_F64`, etc.).
- Add comparison and conditional branching ops (`EQ_I64`, `LT_I64`, `JUMP_IF`).
- Introduce a call stack with proper `CALL` and `RET` semantics.
- Add a constant pool loader (`LOAD_CONST`).
- Provide basic error handling and trap opcodes.

### Phase 2 – Data and Memory Features
- Support arrays with index, load and store instructions.
- Add pointer-style memory access operations.
- Integrate the GC so register roots are traced correctly.
- Enable constants and global value loading across modules.
- Begin optional stack/register hybrid execution support.

### Phase 3 – Advanced and Optional Features
- Implement a register allocator to minimize unused registers.
- Extend the VM to handle dynamic types like strings and booleans.
- Introduce structs and field access operations.
- Provide debugging hooks and execution tracing.

### Phase 4 – Tooling
- Generate opcode metadata for assembler utilities.
- Ship external assembler or disassembly tools.

---

Completing these phases will bring the register VM to feature parity with the stack-based backend while keeping it optimized for future work.
