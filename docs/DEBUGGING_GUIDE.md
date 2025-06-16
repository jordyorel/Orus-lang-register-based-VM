# Debugging Guide

This guide outlines common techniques for debugging Orus programs.

## Enabling Diagnostics

Compile with the `DEBUG=1` flag in the Makefile to include extra
runtime assertions and verbose logging. The interpreter prints
detailed error messages including the file and line of failure.

## Printing Values

Use the `print` builtin to output variable state during execution.
Formatted strings help inspect multiple values:

```orus
print("count={} value={}", count, value)
```

## Tracing Execution

For complex control flow, add logging statements at the start and end
of functions. Pair this with the `range` and `while` loops to ensure
loop variables behave as expected.

## Understanding Errors

Consult `docs/ERROR_REFERENCE.md` for a list of syntax and compile-time
errors. Each error includes hints on how to resolve common mistakes.

## Debugging Generics

When working with generic functions, make sure type constraints are
correct. The `type_of()` builtin can help verify inferred types during
testing.
