# Orus Full Tutorial

This tutorial teaches the Orus language step by step. Each section
shows multiple approaches to the same concept so you can choose the
style that fits your project.

## Hello World

```orus
fn main() {
    print("Hello, Orus!")
}
```

You can also write the entry point using an explicit return:

```orus
fn main() -> i32 {
    print("Hello, Orus!")
    return 0
}
```

## Variables and Types

Declare immutable bindings with `let`:

```orus
let a = 5
let b: f64 = 1.5
```

Add `mut` to allow reassignment:

```orus
let mut count = 0
count = count + 1
```

## Control Flow

For loops iterate over a range:

```orus
for i in range(0, 3) {
    print(i)
}
```

A while loop achieves the same result:

```orus
let mut i = 0
while i < 3 {
    print(i)
    i = i + 1
}
```

## Functions

Functions return the last expression implicitly:

```orus
fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

Alternatively use an explicit `return`:

```orus
fn add(a: i32, b: i32) -> i32 {
    return a + b
}
```

## Structs

Create composite types with `struct`:

```orus
struct Point { x: i32, y: i32 }
```

Instantiate and access fields:

```orus
let p = Point { x: 1, y: 2 }
print(p.x)
```

## Modules and Imports

Split code across files and bring definitions into scope:

```orus
// math.orus
pub fn square(x: i32) -> i32 { x * x }
```

```orus
// main.orus
use math
fn main() {
    print(math.square(3))
}
```

Use `use *` to import everything:

```orus
use math::*
```

## Generics

Generic functions work with many types:

```orus
fn identity<T>(val: T) -> T { val }
```

Type parameters may have constraints:

```orus
fn max<T: Comparable>(a: T, b: T) -> T {
    if a > b { a } else { b }
}
```

## Pattern Matching

`match` expressions select a branch by value:

```orus
match value {
    0 => print("zero"),
    1 => print("one"),
    _ => print("other"),
}
```

## Error Handling

Use `try`/`catch` to deal with failures:

```orus
fn read(path: string) -> string {
    try {
        return std::fs::read_to_string(path)
    } catch err {
        print("failed: {}", err)
        return ""
    }
}
```

## Built-in Helpers

Common functions like `len`, `range` and `timestamp` are always
available:

```orus
let now = timestamp()
print(len([1,2,3]))
```

## Next Steps

Read `docs/ADVANCED_GENERICS_TUTORIAL.md` for deeper coverage of
templates and `docs/DEBUGGING_GUIDE.md` for troubleshooting tips. The
`docs/EXAMPLE_SNIPPETS.md` file offers additional short programs.
