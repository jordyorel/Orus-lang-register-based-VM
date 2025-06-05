# Orus Language Overview

This document provides a short tour of the Orus programming language.
Examples are taken from the programs located in the `tests/` directory.

## Primitive types

Orus supports several built‑in value types:

- `i32` – signed 32‑bit integers.
- `u32` – unsigned 32‑bit integers.
- `f64` – 64‑bit floating point numbers.
- `bool` – boolean values `true` or `false`.
- `string` – UTF‑8 text.
- `nil` – absence of a value, returned from functions without an explicit result.

## Compound types

Two compound type forms are provided:

- **Arrays** – fixed length collections declared with `[T; N]`. Arrays can be
  multidimensional and indexed with `arr[i]`.
- **Structs** – records containing named fields. `impl` blocks may define
  methods or static functions on a struct type.

See `tests/datastructures/stack_queue.orus` for an example mixing arrays,
structs and methods.

### Dynamic arrays

Arrays grow automatically when used with the built‑in `push`, `pop` and `len`
helpers. The initial size declares the starting capacity but new elements may
be appended beyond that limit.

```orus
let numbers: [i32; 1] = [0]
push(numbers, 10)
push(numbers, 20)
print(len(numbers))  // prints 2
let last = pop(numbers)
print(last)
```

## Variables

Variables are introduced with `let` and can optionally specify a type:

```orus
let count = 0
let text: string = "hello"
```
Assignment reuses the variable name on the left hand side.

## Modules

Files can be imported using the `import` statement. The file is executed once
and subsequent imports do nothing.

```orus
import "path/to/module.orus"
```

The imported file can introduce new variables or functions. A simple module
from the test suite prints a greeting:

```orus
# hello_module.orus
fn greet() {
    let greeting = "Hello from module"
    print(greeting)
}

# main.orus
import "tests/modules/hello_module.orus"
fn main() {
    greet()
}
```

All programs must define a `main` function which serves as the entry point for
execution. The interpreter will call this function automatically, so any
top‑level code should be placed inside `fn main()`. Declarations using `let`
and calls to `print` are only allowed inside functions. `import` statements must
appear at the top level of the file, outside of `main`.

## Functions

Functions use the `fn` keyword with an optional return type after `->`:

```orus
fn add(a: i32, b: i32) -> i32 {
    return a + b
}
```

Return statements may be omitted if `nil` is returned. Examples can be found
in `tests/functions/basic_functions.orus` and
`tests/functions/advanced_functions.orus`.

## Control flow

Orus provides common flow control constructs:

- `if`/`elif`/`else` conditional blocks.
- `for` ranges (`for i in 0..10 { ... }`).
- `while` loops.
- `break` and `continue` statements.

All of these are demonstrated under `tests/control_flow/`.

## Methods with `impl`

Methods are defined inside `impl` blocks attached to a `struct` type. Instance
methods receive `self` as the first parameter:

```orus
struct Stack {
    data: [i32; 10],
    top: i32
}

impl Stack {
    fn push(self, value: i32) -> bool {
        /* ... */
    }
}
```

See `tests/datastructures/stack_queue.orus` for full implementations of a stack
and queue using methods.

## Printing values

Use the built‑in `print` function to write output. Strings may include `{}`
placeholders which are replaced by the remaining arguments:

```orus
let name = "Orus"
print("Hello, {}!", name)
```

More examples are available in `tests/print/string_interpolation.orus`.

## Error handling

Orus scripts can catch runtime errors using `try`/`catch` blocks. Code inside
`try` executes normally and if an error occurs control jumps to the matching
`catch` block. The identifier after `catch` receives the error message.

```orus
try {
    // code that might fail
} catch err {
    print("Error: {}", err)
}
```

## Generics

Although the language itself does not currently provide generic syntax,
the interpreter exposes a C macro that generates type‑specific dynamic
array helpers. Defining `DEFINE_ARRAY_TYPE(int, Int)` will create an
`IntArray` struct with `initIntArray`, `writeIntArray` and
`freeIntArray` functions.

```c
DEFINE_ARRAY_TYPE(int, Int);

IntArray nums;
initIntArray(&nums);
writeIntArray(&nums, 42);
freeIntArray(&nums);
```

See `docs/GENERICS.md` for more details.

