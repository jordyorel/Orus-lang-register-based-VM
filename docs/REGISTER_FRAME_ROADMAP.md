# Register VM Call Frame Roadmap

The register-based VM currently defines a `RegisterFrame` type but does not actually
manage a frame stack. The `CALL` opcode only returns a value from the specified
register, preventing nested or recursive calls and causing instability on larger
programs. This roadmap tracks the work required to implement a real call/return
mechanism and improve register allocation for the VM.

---

## 🎯 Goals

- Implement a true call/return model using `RegisterFrame` instances.
- Introduce a minimal register allocator so each function receives a dedicated
  register set.
- Stabilize performance on the comprehensive benchmark.

---

## 🗂️ Phase 1: Frame Semantics

| Task                                                         | Status  |
| ------------------------------------------------------------ | ------- |
| Design `RegisterFrame` layout and lifetime rules             | Done    |
| Add push/pop logic for frames to the VM                      | Done    |
| Update `CALL` and `RETURN` opcodes to use frame stack        | Done |
| Pass arguments through predefined registers                  | Done |

---

## 🛠️ Phase 2: Register Allocation

| Task                                                         | Status  |
| ------------------------------------------------------------ | ------- |
| Replace fixed register ranges with per-function allocation   | Done |
| Support spilling or temporary reuse for recursion            | Done |
| Integrate allocator into the compiler IR generation          | Done |

---

## 📈 Phase 3: Testing & Benchmarking

| Task                                                         | Status  |
| ------------------------------------------------------------ | ------- |
| Add unit tests for nested and recursive function calls       | Done |
| Verify GC traces register frames correctly                   | Done |
| Re-run `benchmarks/compare_vms.sh` to measure improvements   | Done |

---

Once these steps are complete the register VM will support proper call semantics
and be ready for further optimization work.
