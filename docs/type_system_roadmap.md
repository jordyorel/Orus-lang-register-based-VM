
# 🛠️ Orus Type System Roadmap

This roadmap outlines the planned evolution of the **Orus type system** to adopt a **Rust-inspired model** featuring:

- ✅ Immutability by default
- ✅ Explicit mutability using `let mut`
- ✅ Explicit numeric casting using the `as` keyword

> ❌ Deliberately excludes: lifetimes, borrow checker, and trait system

---

## ✅ Phase 1: Immutability by Default

**Goal**: Enforce that all variables are immutable unless explicitly declared as mutable.

### ✔️ Tasks

- [x] Introduce `let mut` syntax for declaring mutable variables
- [x] Disallow reassignment to immutable variables at compile time
- [x] Implement clear error messages for mutability violations

### 🔍 Examples

let a: i32 = 5  
a = 6  
// ❌ Error: cannot assign to immutable variable `a`

let mut b: u32 = 10  
b = 15  
// ✅ OK: `b` is mutable and reassignment is allowed

let mut name: string = "hello"  
name = "world"  
// ✅ OK: mutable reassignment for strings

---

## ✅ Phase 2: Type Casting with `as`

**Goal**: Support **explicit numeric type casting** using the `as` keyword, similar to Rust.

- 🚫 No implicit conversion is allowed between numeric types.
- ✅ All numeric conversions must use `as`.

### ✔️ Tasks

- [x] Implement `as` keyword for type casting
- [x] Support casting between `i32`, `u32`, and `f64`
- [x] Enforce truncation behavior for float → int conversions
- [x] Disallow implicit coercion
- [x] Add parser/compiler support for syntax: `value as Type`

---

### 🔁 Allowed Conversions

| From   | To   | Behavior                          |
|--------|------|-----------------------------------|
| `i32`  | `u32`| Wraps around like Rust            |
| `u32`  | `i32`| Wraps or truncates                |
| `i32`  | `f64`| Normal widening to float          |
| `u32`  | `f64`| Normal widening to float          |
| `f64`  | `i32`| Truncates toward zero             |
| `f64`  | `u32`| Truncates toward zero             |

---

### 🔍 Examples

let mut a: i32 = -42  
let b: u32 = a as u32  
// ✅ b = 4294967254 (wraps around like Rust)

let x: u32 = 123456  
let y: f64 = x as f64  
// ✅ y = 123456.0 (promoted to float)

let z: f64 = -15.9  
let q: i32 = z as i32  
// ✅ q = -15 (decimal truncated, not rounded)

let f: f64 = 42.0  
let n: u32 = f as u32  
// ✅ n = 42

let i: i32 = 99  
let f2: f64 = i as f64  
// ✅ f2 = 99.0

---

## 🚫 Phase 3: Prevent Implicit Conversion

**Goal**: Ensure that no type coercion occurs automatically. All conversions must be explicitly defined using `as`.

### ✔️ Tasks

- [] Block implicit assignment from `f64` to `i32`, `i32` to `u32`, etc.
- [] Block mixed-type arithmetic without `as` casting
- [] Validate types statically at compile time

### 🔍 Invalid Examples

let x: i32 = 3.5  
// ❌ Error: cannot assign float to i32

let x: f64 = 2.0  
let y: i32 = 3  
let z = x + y  
// ❌ Error: cannot mix `f64` and `i32` without cast

let x: f64 = 42.0  
let y: i32 = int(x)  
// ❌ Error: `int()` expects a string, not a float

---

## ⛔ Phase 4: Skipped Rust Features

The following advanced Rust features will **not** be implemented:

- ❌ Borrow checker
- ❌ Lifetime annotations
- ❌ Traits or trait bounds
- ❌ Ownership semantics

---

## 📦 Optional Future Extensions

| Feature            | Description                                 |
|--------------------|---------------------------------------------|
| `as bool`          | Allow casting integers or floats to boolean |
| `as string`        | Allow converting any value to a string      |
| Overflow checking  | Validate casts like `-1 as u8` at runtime   |
| `const` support    | Introduce constant values for compile-time  |

---

## ✅ Summary of the Type System Upgrade

| Feature                               | Status        |
|---------------------------------------|---------------|
| Immutability by default               | ✅ Completed   |
| Explicit `let mut` for reassignment   | ✅ Completed   |
| `as` keyword for type casting         | ✅ Implemented |
| Implicit casting disallowed           | ✅ Enforced    |
| Borrow checker / lifetimes / traits  | ❌ Skipped     |
| Optional future improvements          | 🟡 In planning |

---

## 🔚 Final Notes

This roadmap balances:
- **Safety** (through immutability and explicit casting)
- **Simplicity** (by omitting borrow/lifetime complexity)
- **Power** (via static type checking and future extensibility)

The Orus type system should remain accessible to beginners but strong enough for building robust applications with clear and predictable behavior.
