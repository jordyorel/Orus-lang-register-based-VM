# Orus Language

Orus is an experimental interpreted programming language implemented in C. It features a syntax inspired by modern scripting languages and supports:

- Static type annotations for integers (`i32`/`u32`), floating point numbers (`f64`), booleans and strings.
- Control flow with `if`/`elif`/`else`, `for` and `while` loops as well as `break` and `continue`.
- Functions declared with `fn` and optional return types.
- Arithmetic and comparison operators.
- A `print` function with simple string interpolation using `{}` placeholders.

The repository contains the source code for the interpreter and a collection of sample programs used as tests.

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

Enjoy experimenting with Orus!
