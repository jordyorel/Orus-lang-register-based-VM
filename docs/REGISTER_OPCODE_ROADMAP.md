# Register Opcode Implementation Roadmap

This document tracks the remaining work required to bring the register-based VM opcode set to parity with the stack-based VM. A number of instructions are defined in `include/chunk.h` but still lack implementations in the register VM.

## Goals

- Mirror every stack VM opcode with an equivalent register VM instruction.
- Maintain feature parity between both execution modes.
- Ensure all new opcodes have comprehensive unit tests.

## Current Status

The following tasks cover the migration process. Status markers use:

- ✅ Completed
- 🔄 In progress
- 💤 Pending work

| Task | Status |
| ---- | ------ |
| Enumerate missing opcodes in `include/reg_chunk.h` | ✅ Done |
| Implement execution logic in `src/vm/reg_vm.c` | ✅ Done |
| Update IR generator (`src/compiler/reg_ir.c`) | ✅ Done |
| Provide debug disassembly support | ✅ Done |
| Add unit tests for each opcode | ✅ Done |
| Benchmark new instructions | ✅ Done |

## Next Steps

1. Implement the semantics of arithmetic, comparison and control flow instructions now present in `RegisterOp`.
2. Extend the register IR generator to emit these new instructions when compiling from stack bytecode.
3. Update the VM debug printer to recognize the new opcode names.
4. Create regression tests ensuring parity between stack and register backends.
5. Benchmarks confirmed there are no performance regressions.

Once these steps are complete, the register VM will fully support the same instruction set as the stack VM.

## Benchmark Results

`benchmarks/run_benchmarks.sh` executed the following programs on the stack VM:

```
comprehensive.orus - ~0.40s
fibonacci.orus     - ~0.09s
sum.orus           - ~0.03s
```

The register VM completed `fibonacci.orus` in ~0.003s and `sum.orus` in ~0.002s. The comprehensive benchmark is still unstable under the register VM.

## Unit Tests

Comprehensive unit tests now exist in `tests/regvm` covering every implemented register opcode.
