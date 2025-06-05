# Orus Language

Orus is an experimental interpreted programming language implemented in C. It
features a syntax inspired by modern scripting languages. Supported features
include:

- Static type annotations for integers (`i32`/`u32`), floating point numbers
  (`f64`), booleans and strings with local type inference using `let`.
- Arrays with fixed lengths (including multidimensional arrays) and indexed
  assignment.
- User defined `struct` types and `impl` blocks for instance or static methods.
- Control flow with `if`/`elif`/`else`, `for` ranges and `while` loops as well
  as `break` and `continue`.
- Functions with parameters, recursion and optional return types.
- Arithmetic, comparison and logical (`and`/`or`) operators.
- String concatenation with `+` and a `print` function supporting simple
  interpolation using `{}` placeholders.

The repository contains the source code for the interpreter and a collection of sample programs used as tests. For a quick tour of the language syntax see [`docs/LANGUAGE.md`](docs/LANGUAGE.md).

## Building

A `Makefile` is provided. Building requires `gcc` and `make`.

```sh
# Build the interpreter
make

# Remove generated files
make clean
```

The build process places the final executable in `./orus` and also keeps a copy in `build/release/clox`.

## Running

After building, run the interpreter in two ways:

```sh
# Start an interactive REPL
./orus

# Execute a script file
./orus path/to/script.orus

# Display the current interpreter version
./orus --version
```

## Running the test suite

Tests are located in the `tests/` directory. After building the interpreter, run all tests with:

```sh
bash tests/run_all_tests.sh
```

Each subdirectory of `tests/` represents a category and contains example programs. The script executes every `.orus` file and reports success or failure.

## Repository layout

- `src/` – C source for the interpreter.
- `tests/` – Example programs and regression tests.
- `Makefile` – Build rules producing the `orus` executable.
- `docs/LANGUAGE.md` – Overview of the language syntax.

Enjoy experimenting with Orus!
