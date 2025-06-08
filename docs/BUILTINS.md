# Built-in Functions

Orus provides several built-in functions that are always available without
needing to import a module. These functions handle common tasks like printing
values or working with arrays. The table below lists each function with a brief
description.

| Function | Description |
|----------|-------------|
| `print(values...)` | Output values to the console using `{}` placeholders for interpolation. |
| `len(value)` | Return the length of a string or array. |
| `substring(str, start, len)` | Extract a portion of a string. |
| `push(array, value)` | Append a value to a dynamic array. |
| `pop(array)` | Remove and return the last element of an array. |
| `range(start, end)` | Return an array of integers from `start` (inclusive) to `end` (exclusive). |
| `sum(array)` | Sum all numeric elements of an array.
| `type_of(value)` | Return the name of a value's type as a string. |
| `is_type(value, name)` | Check whether a value is of the given type. |
| `input(prompt)` | Display a prompt and return a line of user input. |
| `int(text)` | Convert a string to an `i32`, raising an error on failure. |
| `float(text)` | Convert a string to an `f64`, raising an error on failure. |

These built-ins form the core of the standard library and are sufficient for
basic programs. More utility functions may be added in the future.
