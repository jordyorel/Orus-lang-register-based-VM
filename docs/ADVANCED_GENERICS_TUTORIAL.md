# Advanced Generics Tutorial

This tutorial expands on the basics covered in `docs/GENERICS.md` and
introduces more complex patterns using the generics system in Orus.

## Type Constraints Recap

Generic parameters may declare constraints such as `Numeric` or
`Comparable`. These enable arithmetic or comparison operators for the
target types.

```orus
fn max<T: Comparable>(a: T, b: T) -> T {
    if a > b { a } else { b }
}
```

## Cross-Module Generics

Generic structs and functions can be imported from other modules. The
compiler specializes them on demand when used in a new module.

```orus
use utils::collections::Map

let table = Map<string, i32>::new()
```

## Collections and Iterators

The standard library provides generic `Map` and `Set` types along with
iterator helpers like `map` and `filter`. These utilities showcase how
generics simplify container code.

## Debugging Generic Code

When errors occur, check the inferred type arguments using `type_of()`
and ensure that all constraints are satisfied. The debugging guide
lists additional strategies for tracing template instantiations.
