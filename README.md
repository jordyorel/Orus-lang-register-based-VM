# Orus Language

Orus is an experimental interpreted programming language implemented in C. It
features a syntax inspired by modern scripting languages. Supported features
include:

- Static type annotations for integers (`i32`/`u32`), floating point numbers
  (`f64`), booleans and strings with local type inference using `let`.
- Arrays with fixed lengths (including multidimensional arrays) and
  dynamic arrays using `push`, `pop` and `len`.
- User defined `struct` types and `impl` blocks for instance or static methods.
- Generic functions and structs with type parameters.
- Modules and `import` statements for splitting code across files.
- Control flow with `if`/`elif`/`else`, `for` ranges and `while` loops as well
  as `break` and `continue`.
- Error handling with `try`/`catch` blocks.
- Functions with parameters, recursion and optional return types.
- Function calls can reference functions defined later in the file.
- Arithmetic, comparison and logical (`and`/`or`/`not`) operators.
- String concatenation with `+` and a `print` function supporting simple
  interpolation using `{}` placeholders.
- Macro helpers for generic dynamic arrays.

The repository contains the source code for the interpreter and a collection of sample programs used as tests. For a quick tour of the language syntax see [`docs/LANGUAGE.md`](docs/LANGUAGE.md). Additional notes on the generics and array helper are available in [`docs/GENERICS.md`](docs/GENERICS.md). A future compilation roadmap is outlined in [`docs/COMPILATION_ROADMAP.md`](docs/COMPILATION_ROADMAP.md).

## Building

A `Makefile` is provided. Building requires `gcc` and `make`.

```sh
# Build the interpreter
make

# Remove generated files
make clean
```

### Platform notes

* **Linux** and **macOS** – Install `gcc`, `make` and `python3` with your
  package manager (for macOS you may first need the Xcode command line tools).
  Then run the commands above from a terminal.

* **Windows** – Use [WSL](https://learn.microsoft.com/windows/wsl/) or an MSYS2
  environment so that `make` and `gcc` are available. After installing the
  tools, build from the provided shell just like on Linux.


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

The script uses `bash`. On Windows, run it from WSL or Git Bash so that the
shell utilities behave like they do on Unix systems.

Each subdirectory of `tests/` represents a category and contains example programs. The script executes every `.orus` file and reports success or failure.

## Benchmarking

Simple micro benchmarks live in the `benchmarks/` directory. After building the
interpreter run:

```sh
bash benchmarks/run_benchmarks.sh
```

The script executes each benchmark program and prints the time spent running it.

## Development tools

A basic package manager is provided to help build and distribute Orus
projects. The `tools/oruspm.py` script can initialise a new project,
compile the interpreter, run the entry file and create a distributable
archive.

```sh
# Create a new project in the current directory
python3 tools/oruspm.py init my_project

# Compile the interpreter
python3 tools/oruspm.py build

# Build and run the entrypoint
python3 tools/oruspm.py run

# Create `dist/<name>-<version>.tar.gz`
python3 tools/oruspm.py pack
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
- `std/` - Standard library components
- `docs/` - Documentation
  - `LANGUAGE.md` – Overview of the language syntax
  - `GENERICS.md` – Notes on the generic programming capabilities
  - `COMPILATION_ROADMAP.md` – Planning for future compilation features
- `tools/` – Utility scripts including the `oruspm.py` package manager.

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
