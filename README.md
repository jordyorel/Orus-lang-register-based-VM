# Orus Language

Orus is an experimental interpreted programming language implemented in C. It
features a syntax inspired by modern languages such as Python and Rust. Supported features
include:

- Static type annotations for integers (`i32`/`u32`/`i64`/`u64`), floating point numbers
  (`f64`), booleans and strings with local type inference using `let`.
- Arrays with fixed lengths (including multidimensional arrays) and
  dynamic arrays using `push`, `pop`, `len`, `sum`, `min` and `max`.
- User defined `struct` types and `impl` blocks for instance or static methods.
- Public structs using `pub struct` to expose types from modules.
- Generic functions and structs with type parameters, including cross-module generics.
- Modules and `use` statements for splitting code across files.
- Control flow with `if`/`elif`/`else`, `for` ranges and `while` loops as well
  as `break` and `continue`.
- A `range(start, end)` syntax for `for` loops to iterate over a sequence of
  integers.
- Pattern matching with `match` statements.
- Error handling with `try`/`catch` blocks.
- Functions with parameters, recursion and optional return types.
- Function calls can reference functions defined later in the file.
- Arithmetic, comparison and logical (`and`/`or`/`not`) operators.
- Inline if expressions with the `?:` operator.
- String concatenation with `+` and a `print` function supporting simple
  interpolation using `{}` placeholders. The `print` function automatically
  appends a newline after the formatted output.
- User input via `input(prompt)` for interactive programs.
- Macro helpers for generic dynamic arrays.
- Compile-time constants defined with the `const` keyword.
- Standard library modules under `std/` such as `std/math` for math utilities, `std/datetime` for working with dates and times, and `std/collections` for generic data structures.
- `DateTime` values automatically display in `YYYY-MM-DD HH:MM:SS` format when printed.
- Variables are immutable by default. Use `let mut` for reassignment.
  See [docs/MUTABILITY.md](docs/MUTABILITY.md) for a detailed explanation.
- Explicit numeric casting with the `as` keyword; no implicit conversions.
- Booleans convert to numbers as `1` or `0` and any numeric type may be cast
  to `bool`.
- All primitive values can be converted to `string` with `as string`. Casting
  from `string` or `nil` to numeric types is disallowed.
- Integer literals automatically use `i32`, `i64` or `u64` based on value. A
  trailing `u` suffix forces an unsigned type.
- Local variables without an explicit type annotation default to `i64` when the
  initializer is an integer expression.

The repository contains the interpreter sources and a large suite of example programs. A quick tour of the syntax lives in [`docs/LANGUAGE.md`](docs/LANGUAGE.md) and a step-by-step introduction is provided in [`docs/TUTORIAL.md`](docs/TUTORIAL.md). Notes on generics appear in [`docs/GENERICS.md`](docs/GENERICS.md) and more examples are spread throughout the test suite. See [`docs/TESTS_OVERVIEW.md`](docs/TESTS_OVERVIEW.md) for a list of categories. A potential compilation roadmap is outlined in [`docs/COMPILATION_ROADMAP.md`](docs/COMPILATION_ROADMAP.md). For a summary of built-in functions consult [`docs/BUILTINS.md`](docs/BUILTINS.md).

## Building

A `Makefile` is provided. Building requires `gcc` and `make`.

```sh
# Build the interpreter
make

# Remove generated files
make clean
```

### Platform notes

* **Linux** and **macOS** – Install `gcc` and `make` with your
  package manager (for macOS you may first need the Xcode command line tools).
  Then run the commands above from a terminal.

* **Windows** – Use [WSL](https://learn.microsoft.com/windows/wsl/) or an MSYS2
  environment so that `make` and `gcc` are available. After installing the
  tools, build from the provided shell just like on Linux.


The build process places the final executable in `./orusc` and also keeps a copy in `build/release/clox`.

## Running

After building, run the interpreter in two ways:

```sh
# Start an interactive REPL
./orusc

# Execute a script file using the register-based VM
./orusc path/to/script.orus

# Execute a project directory
./orusc --project path/to/project

To trace individual register updates during execution, compile with
`DEBUG_TRACE_EXECUTION` enabled in `reg_vm.c` and run the interpreter with
the `--trace` flag or by setting `ORUS_TRACE=1`.

When debugging array access issues you can additionally enable
`DEBUG_ARRAY_INDEX` in `reg_vm.c` to log the chosen index and array length
for every `ROP_ARRAY_GET` or `ROP_ARRAY_SET` instruction.

```

When run in project mode the interpreter searches all `.orus` files for a
`main` function if the manifest doesn't specify an entry file. The project must

contain exactly one such function.

For additional runtime options see [docs/ENVIRONMENT.md](docs/ENVIRONMENT.md).

# Display the current interpreter version

./orusc --version
```

## Running the test suite

Tests are located in the `tests/` directory. After building the interpreter, run all tests with:

```sh
bash tests/run_all_tests.sh
```

The script uses `bash`. On Windows, run it from WSL or Git Bash so that the
shell utilities behave like they do on Unix systems.

Each subdirectory of `tests/` represents a category and contains example programs. The script executes every `.orus` file and reports success or failure.
For a summary of each category see `docs/TESTS_OVERVIEW.md`.

## Benchmarking

Simple micro benchmarks live in the `benchmarks/` directory. It also contains `comprehensive.orus` which exercises datetime handling, generic types, math utilities, random number generation and collection helpers. After building the interpreter run:

```sh
bash benchmarks/run_benchmarks.sh
```

The script executes each benchmark program and prints the time spent running it.
Each benchmark also reports its own elapsed time using `timestamp()` so they can
be executed individually and still provide timing information.

Run the benchmark suite with:

```sh
bash benchmarks/compare_vms.sh
```

## Repository layout

- `src/` – C source for the interpreter.
  - `compiler/` - Compiler components for syntax analysis
  - `parser/` - Parsing implementation
  - `scanner/` - Lexical analysis
  - `util/` - Utility functions
  - `vm/` - Virtual machine implementation
- `tests/` – Example programs and regression tests.
  - `algorithms/` - Algorithm implementations
  - `arithmetic/` - Arithmetic operation tests
  - `builtins/` - Tests for built-in functions
  - `control_flow/` - Tests for if/else/loops/try-catch
  - `datastructures/` - Data structure implementations
  - `edge_cases/` - Edge case testing
  - `errors/` - Error handling tests
  - `generics/` - Tests for generic functionality
- `include/` - Header files for language components
  - `std/` - Standard library modules such as `std/math`
- `docs/` - Documentation
  - `LANGUAGE.md` – Overview of the language syntax and best practices
  - `GENERICS.md` – Notes on the generic programming capabilities
  - `COMPILATION_ROADMAP.md` – Planning for future compilation features
  - `REGISTER_VM_ROADMAP.md` – Steps for migrating to a register-based VM
  - `REGISTER_OPCODE_ROADMAP.md` – Progress on implementing missing opcodes
  - `REGISTER_USAGE.md` – Register allocation conventions
  - `DEBUGGING_GUIDE.md` – Tips for diagnosing and fixing issues
  - `ADVANCED_GENERICS_TUTORIAL.md` – In-depth guide for generic code
  - `EXAMPLE_SNIPPETS.md` – Short sample programs
- `tools/` – Utility scripts.

## Future Development

Work is currently underway to enhance Orus with the following:

1. **Improved Generics**: Expanding type constraints and generic data structures (see `docs/GENERICS.md`)
2. **Potential Compilation**: Exploration of adding compilation capabilities (see `docs/COMPILATION_ROADMAP.md`)
3. **Standard Library Expansion**: Building out more comprehensive standard libraries

## Contributing

1. Fork the project and create a feature branch.
2. Build the interpreter with `make` and run `bash tests/run_all_tests.sh`.
3. Open a pull request describing your changes.

Development works best on Linux or macOS. On Windows, use WSL or an MSYS2 shell
so that `gcc`, `make` and `bash` behave like they do on Unix systems.

Enjoy experimenting with Orus!
