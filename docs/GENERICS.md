<!-- filepath: /Users/hierat/Documents/Development/learning/orus_lang/docs/GENERICS.md -->
# Generics Implementation Todo List

## Current Status
- [x] Basic generic functionality implemented
- [x] Generic types for collections via C-side macros (`DEFINE_ARRAY_TYPE`)
- [x] Regular function forward declarations supported
- [x] Generic function forward declarations supported
- [ ] Full generics implementation (in progress)

## High Priority Tasks

### Generic Forward Declarations & Prepass Collection
- [x] Regular (non-generic) forward declarations are already supported
- [x] Implement prepass mechanism to collect generic function signatures
- [x] Remove generic function definition order restrictions
- [x] Add tests for generic forward declaration functionality
- [x] Document the prepass collection process

### Generic Constraints
- [x] Design constraint syntax (e.g., `T: Numeric`, `T: Comparable`)
- [x] Implement compile-time checks for constrained operations
- [x] Expand the `type_constraints.orus` test with more use cases
- [x] Document constraint system in language guide

Generic parameters may declare constraints after the name:

```
fn min<T: Comparable>(a: T, b: T) -> T { ... }
fn add<T: Numeric>(a: T, b: T) -> T { ... }
```

`Numeric` enables arithmetic, bitwise and negation operators. `Comparable`
allows comparison and equality operations. Numeric types implicitly satisfy
`Comparable`.

### Improved Type Inference
- [x] Enhance inference for arguments in generic functions
- [x] Add support for nested generics inference
- [ ] Optimize inference for complex expressions
- [ ] Write test cases for edge cases in type inference

## Medium Priority Tasks

-### Generic Arithmetic and Operators
- [x] Design trait-based system for numeric operations
- [x] Implement operator overloading for generic types
- [x] Replace specialized implementations (e.g., `sum` for `[i32]`) with generic versions
- [x] Create comprehensive tests for generic arithmetic

### Collection and Iterator Support
- [x] Build generic Map implementation
- [x] Build generic Set implementation
- [x] Implement standard iterator protocol for collections
- [x] Create higher-order functions for collections (map, filter, reduce)

### Error Reporting
- [ ] Improve error messages for generic type mismatches
- [ ] Add forward declaration suggestions to relevant error messages
- [ ] Include examples in compile-time errors
- [ ] Create user-friendly debugging guide for generics

## Lower Priority Tasks

### Cross-Module Generics
- [ ] Enable importing generic types from other modules
- [ ] Ensure proper specialization across compilation units
- [ ] Create tests with multi-file scenarios
- [ ] Document module-boundary behavior

### Documentation and Examples
- [ ] Create advanced generics tutorial
- [ ] Document best practices and patterns
- [ ] Add code examples for common use cases
- [ ] Update docs when new features are implemented

## Implementation Notes
* Current progress: generic functions and structs support forward declarations through a prepass.
* Current limitations: cross-module specialization is incomplete and arithmetic support is still evolving.
* Reference the `tests/generics/` directory for existing test cases
* Prioritize improving developer experience with better error messages
* Consider performance implications of specialization vs. type erasure
* Basic collection types and iterators are provided in `std/collections`
