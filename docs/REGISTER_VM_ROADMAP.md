# Orus Register-Based VM Roadmap

This roadmap outlines the long-term migration of the Orus virtual machine architecture from a stack-based design to a **register-based VM**, enabling significantly faster execution of statically typed code.

---

## 🎯 Goal

Design and implement a high-performance, register-based virtual machine for Orus to:

* Reduce instruction dispatch overhead.
* Eliminate stack thrashing.
* Enable more aggressive optimization of statically typed code.

---

## 🧱 Key Advantages of a Register-Based VM

| Feature                | Stack-Based VM           | Register-Based VM          |
| ---------------------- | ------------------------ | -------------------------- |
| Instruction Count      | Higher (due to push/pop) | Lower (direct access)      |
| Operand Access         | Indirect via stack       | Direct via registers       |
| Code Density           | Lower                    | Higher                     |
| Optimization Potential | Limited                  | High (can inline, reorder) |
| Loop Performance       | Slower                   | Faster                     |

---

## 🛠️ Phase 1: Design Foundations

| Task                                          | Status  |
| --------------------------------------------- | ------- |
| Register-based bytecode format                | Done    |
| Define a fixed-size virtual register file     | Done    |
| Implement IR generator that maps to registers | Done    |
| Replace VM stack with register file           | Done    |

---

## ⚙️ Phase 2: Core Instruction Set

| Task                                                    | Status  |
| ------------------------------------------------------- | ------- |
| Implement `MOV`, `LOAD_CONST` and type-specific arithmetic ops using a 3-operand format | Done |
| Implement typed comparison ops: `EQ_I64`, `GT_I64`, etc | Done |
| Update control flow ops (`JUMP`, `JZ`, `CALL`)          | Done |
| Update loop constructs to register-indexed form         | Done |

---

## 🧪 Phase 3: Interpreter Engine

| Task                                               | Status  |
| -------------------------------------------------- | ------- |
| Rewrite dispatch loop for register-based execution | Done |
| Implement register allocator and lifetime analysis | Done |
| Optimize dispatch table (e.g., direct threading)   | Done |
| Benchmark vs current stack-based VM                | Done |

---

## 🧼 Phase 4: Integration & Compatibility

| Task                                               | Status  |
| -------------------------------------------------- | ------- |
| Enable dual-backend (stack/register) for testing   | Done |
| Port all built-in functions to register model      | Done |
| Update compiler backend to emit register bytecode  | Done |
| Ensure GC compatibility with register-based frames | Planned |

---

## 📦 Example Register-Based Bytecode (Hypothetical)

```orus
fn main() {
    let mut sum = 0
    for i in 0..1_000_000 {
        sum = sum + i
    }
    print(sum)
}
```

Might Compile To:

```
LOAD_CONST R1, 0          ; sum = 0
LOAD_CONST R2, 0          ; i = 0
LOAD_CONST R3, 1000000    ; limit

LOOP_START:
CMP_LT R2, R3 -> R4       ; if i < limit
JZ R4, LOOP_END
ADD R1, R2 -> R1          ; sum += i
ADD_CONST R2, 1 -> R2     ; i += 1
JUMP LOOP_START

LOOP_END:
PRINT R1
```

---

## 🔚 Final Goal

Make Orus execution model future-proof, unlock JIT/AOT potential, and enable microsecond-level execution of numeric-heavy code by eliminating interpretation overhead.

---
