# Test Suite Overview

The `tests/` directory contains nearly two hundred example programs used as
regression tests. Each subdirectory groups related language features. Running
`bash tests/run_all_tests.sh` executes every test file.

| Directory | Purpose |
|-----------|---------|
| `algorithms` | Classic algorithm implementations |
| `arithmetic` | Arithmetic operations |
| `builtins` | Built-in function behavior |
| `comparison` | Comparison operators and logic |
| `constants` | Compile-time and static constants |
| `control_flow` | If/else, loops and break/continue |
| `datastructures` | Arrays, maps and sets |
| `edge_cases` | Unusual or tricky language constructs |
| `errors` | Compile-time and runtime error cases |
| `functions` | Function definitions and calls |
| `gc` | Garbage collection scenarios |
| `generics` | Generic functions and structs |
| `main` | Entry point discovery |
| `match` | Pattern matching |
| `modules` | Module imports |
| `projects` | Full project directories |
| `stdlib` | Standard library modules |
| `strings` | String handling |
| `structs` | Struct definitions |
| `types` | Type system features |
| `variables` | Variable declarations |

These tests double as tutorial examples. Browse a category to see short 
programs that exercise each capability. The `projects/` directory shows how 
multiple modules work together under a manifest file.
