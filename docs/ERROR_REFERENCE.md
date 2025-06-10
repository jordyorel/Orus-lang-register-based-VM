# Orus Error Reference

This document consolidates the syntax and compile-time errors currently emitted by the Orus language tools (version 0.5.2). Syntax errors arise while scanning and parsing source files. Compile-time errors occur later during type checking and semantic analysis.

## Syntax errors

### Lexical errors
- **Unterminated block comment** – reached end of file without closing `/* */`.
- **Invalid underscore placement in number** – underscores may only appear between digits.
- **Invalid scientific notation** – missing digits after the exponent indicator.
- **Invalid escape sequence** – unrecognised `\\` escape inside string literals.
- **Unterminated string** – string literal reaches end of file without a closing quote.
- **Unexpected character** – any character not recognised by the scanner.

### Parser errors
The parser expects specific tokens at each point in the grammar. When a token does not match the expectation, one of these messages is issued:
- `Can only call functions.`
- `Unexpected end of file.`
- `Expected expression.`
- `Invalid infix operator.`
- `Semicolons are not used in this language. Use newlines to terminate statements.`
- `Expect newline after statement.`
- `Expect ')' after expression.`
- `Expect ')' after arguments.`
- `Expect ']' after slice expression.`
- `Expect ']' after index expression.`
- `Expect property or method name after '.'.`
- `Expect '>' after generic arguments.`
- `Expect ']' after array elements.`
- `Expect field name.`
- `Expect ':' after field name.`
- `Expect '}' after struct literal.`
- `Expect iterator variable name.`
- `Expect 'in' after iterator variable.`
- `Expect '(' after 'range'.`
- `Expect ',' after range start.`
- `Expect ')' after range.`
- `Expect '..' in range expression.`
- `Expect '{' after match value.`
- `Expect '=>' after pattern.`
- `Expect '}' after match cases.`
- `Expect 'catch' after try block.`
- `Expect identifier after 'catch'.`
- `Expect function name.`
- `Expect generic parameter name.`
- `Expect '>' after generic parameters.`
- `Expect '(' after function name.`
- `Expect parameter name.`
- `Expect ':' after parameter name.`
- `Expect ')' after parameters.`
- `Expect '{' after function return type.`
- `'return' outside of function.`
- `'print' outside of function.`
- `First argument to print must evaluate to a string for interpolation.`
- `Expected expression as argument.`
- `Expect ')' after print arguments. (Hint: string arguments must be quoted)`
- `Expect ')' after print argument. (Hint: string arguments must be quoted)`
- `Expected 'fn' after 'pub'.`
- `'import' must be at top level.`
- `'use' must be at top level.`
- `'let' declarations must be inside a function.`
- `Expect variable name.`
- `Expect '=' after variable name.`
- `Invalid assignment target.`
- `Unknown type name.`
- `Expected type name (i32, u32, f64, bool, string or struct).`
- `Failed to get primitive type.`
- `Expect type name after impl.`
- `Expect '{' after impl name.`
- `Expect '}' after impl block.`
- `Expect struct name.`
- `Expect '{' after struct name.`
- `Expect '}' after struct fields.`
- `Expect alias after 'as'.`
- `Expect alias name.`
- `Expect symbol name.`
- `Expect '}' after symbol list.`
- `Expect symbol name after '::'.`
- ``import` statements are deprecated; use `use module::path` instead``
- `Expect module path after 'use'.`
- `Expect module path string after 'import'.`

## Compile-time errors

### Variable and scope issues
- **Undefined variable** – no declaration for the referenced name can be found in the current or enclosing scopes.
- **Redeclaration** – a variable with the same name already exists in the current scope.
- **Immutable assignment** – attempting to assign to a variable not declared with `mut`.
- **Variable type inference failed** – the compiler could not deduce a variable's type from its initializer.

### Type checking errors
- **Type mismatch** – the type of an expression does not match the expected type (e.g. assigning `f64` to `i32`).
- **Invalid cast** – an explicit `as` conversion between incompatible types.
- **Invalid operand types** – using unsupported types with unary or binary operators.
- **Unknown struct type** – referencing a struct that has not been defined.
- **Struct literal field issues** – missing fields or mismatched field types when constructing a struct.
- **Field access on non‑struct** – using the `.` operator on a value that is not a struct.
- **Array element type mismatch** – elements of an array literal do not all share the same type.

### Function and module errors
- **Undefined function** – calling a function that is not in scope or has not been imported.
- **Private access** – attempting to use a function or variable that is marked `pub` in another module.
- **Incorrect argument count** – calling a function with too few or too many arguments.
- **Built‑in argument type error** – wrong types supplied to built‑in functions such as `len()` or `substring()`.

### Control flow errors
- **Non‑boolean condition** – `if`, `elif` and `while` conditions must evaluate to `bool`.
- **Invalid `break`/`continue`** – these statements may only appear inside loops.
- **Invalid `for` range values** – `range` arguments must be integers.

### Miscellaneous errors
- **Too many functions or variables** – exceeded internal compiler limits.
- **Memory allocation failure** – the compiler could not allocate memory for a symbol name.
- **Unsupported AST node** – the compiler encountered a construct that is not yet implemented.
- **Missing `main` function** – projects executed in project mode must define exactly one `main` entry point.

These messages capture the common diagnostics currently emitted by the Orus tools. Each error includes a source span and optional notes to help pinpoint the offending code.

## Runtime errors

Runtime errors produced by the interpreter now follow the same diagnostic style as compile-time errors. In addition to the error message, a helpful suggestion and note are printed to aid debugging.

Some typical runtime diagnostics are:

- **Stack underflow** – an operation was executed without enough values on the stack. *Suggestion:* ensure all operands are pushed before the operator. *Note:* this usually means a value was omitted or an expression was removed.
- **Module `path` not found** – the interpreter could not locate an imported module. *Suggestion:* verify the import path or set the `ORUS_STD_PATH` environment variable. *Note:* module paths are resolved relative to the current file or the standard library directory.
- **Import cycle detected** – two modules depend on each other. *Suggestion:* restructure your modules to break the cycle. *Note:* each module's top-level code runs only once.
- **Too few arguments for string interpolation** – the number of `{}` placeholders does not match the provided values. *Suggestion:* add or remove placeholders or arguments so they match. *Note:* every `{}` corresponds to one argument after the format string.

These runtime messages provide clear guidance on how to resolve common mistakes when running Orus programs.
