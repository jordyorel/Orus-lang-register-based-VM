# Register VM Feature Expansion Roadmap

This document tracks capabilities still missing from the register-based virtual machine and outlines a phased plan to implement them. Most items in **Phase 1** and **Phase 2** have now been completed, with further work planned for later phases.

## 🧩 Updated Missing Features Checklist

| Limitation | Can Be Optimized? | Notes |
|-----------|-------------------|------|
| ✅ i64 integer arithmetic | ✅ Done | Already implemented (ADD_I64, etc.) |
| ✅ f64 floating-point support | ✅ Done | Implemented via ADD_F64, MUL_F64 and typed registers |
| ✅ Function call stack & return values | ✅ Done | CALL/RETURN with register frames |
| ✅ Arrays and indexing | ✅ Done | ARRAY_GET/SET plus push/pop operations |
| ✅ Pointer or memory access ops | ✅ Done | Added LOAD_PTR/STORE_PTR instructions |
| ✅ Register allocator | ✅ Done | Per-function allocation in reg_ir.c |
| ✅ Dynamic type support (e.g., strings) | ✅ Done | Registers store strings, bool and nil |
| ✅ GC integration | ✅ Done | Register roots marked during collection |
| ✅ Structs / field access | ✅ Done | Fields compiled to array slots |
| ✅ Constants pool / global values | ✅ Done | LOAD_CONST and global load/store |
| ✅ Comparison and branching | ✅ Done | EQ_I64, LT_I64, JUMP_IF, etc. |
| ✅ Stack-based fallback or hybrid mode | ✅ Done | Mixed execution supported for legacy compatibility |
| ✅ Error handling / traps | ✅ Done | Try/catch with SETUP_EXCEPT and POP_EXCEPT |
| ❌ Debugging hooks / tracing | 💤 Planned | Could add TRACE, line numbers, or source tracking |
| ❌ Opcode metadata or assembler tools | 💤 External tooling | Not essential now, but will matter for maintainability |

## 🛣️ Roadmap

### Phase 1 – Core VM Capabilities
- ✅ Implement `f64` arithmetic instructions (`ADD_F64`, `SUB_F64`, etc.).
- ✅ Add comparison and conditional branching ops (`EQ_I64`, `LT_I64`, `JUMP_IF`).
- ✅ Introduce a call stack with proper `CALL` and `RET` semantics.
- ✅ Add a constant pool loader (`LOAD_CONST`).
- ✅ Provide basic error handling and trap opcodes.

### Phase 2 – Data and Memory Features
- ✅ Support arrays with index, load and store instructions.
- ✅ Add pointer-style memory access operations.
- ✅ Integrate the GC so register roots are traced correctly.
- ✅ Enable constants and global value loading across modules.
- ✅ Begin optional stack/register hybrid execution support.

### Phase 3 – Advanced and Optional Features
- ✅ Implement a register allocator to minimize unused registers.
- ✅ Extend the VM to handle dynamic types like strings and booleans.
- ✅ Introduce structs and field access operations.
- ❌ Provide debugging hooks and execution tracing.

### Phase 4 – Tooling
- ❌ Generate opcode metadata for assembler utilities.
- ❌ Ship external assembler or disassembly tools.

---

Completing these phases will bring the register VM to feature parity with the stack-based backend while keeping it optimized for future work.
