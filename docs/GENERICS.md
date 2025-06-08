<!-- filepath: /Users/hierat/Documents/Development/learning/orus_lang/docs/GENERICS.md -->
# Generics in Orus Language

This document outlines the current state and future roadmap for generic data structures and functions in the Orus language.

## Current Implementation

Currently, Orus supports basic generic functionality with some limitations. Generic types can be used for collections and functions, primarily through C-side macros like `DEFINE_ARRAY_TYPE` for arrays.

## Roadmap for Future Development

Based on the codebase analysis and implementation testing, the following enhancements are planned:

### 1. Forward Declarations & Prepass Collection

**Goal**: Allow calling generic functions before their definitions.
- Implement a prepass mechanism to collect function signatures
- Remove the current order restriction for generic function definitions
- Enable more natural code organization

### 2. Generic Constraints

**Goal**: Ensure type safety for generic operations.
- Add constraint systems (e.g., `T: Numeric`, `T: Comparable`)
- Provide compile-time checks for operations like `<`, `>`, or `+`
- Reference implementation in `type_constraints.orus` test

### 3. Improved Type Inference

**Goal**: Reduce explicit type annotations.
- Enhance inference for arguments in generic functions
- Improve handling of nested generics
- Support complex expressions with generics

### 4. Generic Arithmetic and Operators

**Goal**: Support arithmetic operations across generic types.
- Implement trait-based constraints for numeric operations
- Enable operator overloading for generic types
- Remove need for specialized implementations (e.g., current `sum` for `[i32]`)

### 5. Better Collection and Iterator Support

**Goal**: Provide a rich ecosystem of generic collections.
- Build on existing C-side macros for arrays
- Implement higher-level generic collections (maps, sets)
- Add comprehensive iterator support for all collections

### 6. Error Reporting Enhancements

**Goal**: Improve developer experience with clearer error messages.
- Provide better diagnostics for generic type mismatches
- Add suggestions for using forward declarations when appropriate
- Include helpful examples in error messages

### 7. Cross-Module Generic Types

**Goal**: Support generics across module boundaries.
- Enable importing generic types from other modules
- Ensure specialization works correctly across compilation units
- Test with multi-file scenarios

### 8. Tooling and Documentation

**Goal**: Support developers with comprehensive resources.
- Expand this documentation with complex patterns and best practices
- Provide examples for advanced generic usage
- Update as new features are implemented

## Conclusion

This roadmap aims to make generics in Orus more ergonomic and expressive while addressing the current function-definition order constraint. These improvements will enable more powerful abstractions and safer code.