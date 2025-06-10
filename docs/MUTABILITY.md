# Mutability in Orus

Orus embraces immutability as the default to make programs easier to reason about. Variables declared with `let` cannot be reassigned unless explicitly marked as mutable with `let mut`.

This document explains how mutability works in Orus, what errors you might see, and provides several small examples.

## Immutable by Default

When you use `let`, the variable binding is immutable. Attempting to assign to it later results in a compile-time error.

```orus
fn main() {
    let x: i32 = 5
    x = 6  // ❌ Error
}
```

Compilation produces an error message similar to:

```
Error: cannot assign to immutable variable `x`
```

## Declaring Mutable Variables

Use `let mut` when you need to change the value of a variable:

```orus
fn main() {
    let mut count: i32 = 0
    count = 10        // ✅ OK
    count += 2        // ✅ OK
}
```

A variable marked `mut` can be reassigned any number of times, but its **type** cannot change after declaration.

```orus
fn main() {
    let mut value: i32 = 1
    value = 2         // OK
    value = 3.0       // ❌ Error: type mismatch
}
```

## Mutating Data Structures

Mutability controls whether the variable binding can change. The contents of arrays or structs may still be modified even if the binding itself is immutable.

```orus
fn main() {
    let numbers: [i32; 1] = [0]  // binding is immutable
    push(numbers, 42)            // allowed: array contents updated
    // numbers = [1]            // Error: cannot reassign `numbers`
}
```

For struct fields, the variable must be `mut` to modify its fields:

```orus
struct Point { x: i32, y: i32 }

fn main() {
    let mut p = Point{ x: 0, y: 0 }
    p.x = 3        // ✅ allowed
    p.y = 4
}
```

If `p` were declared without `mut`, assigning to `p.x` or `p.y` would fail.

## Summary

- Variables are **immutable by default**. Attempting to reassign them produces a compile-time error.
- Use **`let mut`** to create a mutable binding.
- Mutable bindings still keep the **same type** for their lifetime.
- The mutability of a binding is separate from the mutability of the data it references.

See `tests/errors/immutable_assignment.orus` for an example test of the compiler rejecting reassignment to an immutable variable.
