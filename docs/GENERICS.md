<!-- filepath: /Users/hierat/Documents/Development/learning/orus_lang/docs/GENERICS.md -->
# Generics Implementation Todo List

## Current Status
- [x] Basic generic functionality implemented
- [x] Generic types for collections via C-side macros (`DEFINE_ARRAY_TYPE`)
- [ ] Full generics implementation (in progress)

## High Priority Tasks

### Forward Declarations & Prepass Collection
- [ ] Implement prepass mechanism to collect function signatures
- [ ] Remove function definition order restrictions
- [ ] Add tests for forward declaration functionality
- [ ] Document the prepass collection process

### Generic Constraints
- [ ] Design constraint syntax (e.g., `T: Numeric`, `T: Comparable`)
- [ ] Implement compile-time checks for constrained operations
- [ ] Expand the `type_constraints.orus` test with more use cases
- [ ] Document constraint system in language guide

### Improved Type Inference
- [ ] Enhance inference for arguments in generic functions
- [ ] Add support for nested generics inference
- [ ] Optimize inference for complex expressions
- [ ] Write test cases for edge cases in type inference

## Medium Priority Tasks

### Generic Arithmetic and Operators
- [ ] Design trait-based system for numeric operations
- [ ] Implement operator overloading for generic types
- [ ] Replace specialized implementations (e.g., `sum` for `[i32]`) with generic versions
- [ ] Create comprehensive tests for generic arithmetic

### Collection and Iterator Support
- [ ] Build generic Map implementation
- [ ] Build generic Set implementation
- [ ] Implement standard iterator protocol for collections
- [ ] Create higher-order functions for collections (map, filter, reduce)

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
* Current limitations: function order restrictions, limited arithmetic support
* Reference the `tests/generics/` directory for existing test cases
* Prioritize improving developer experience with better error messages
* Consider performance implications of specialization vs. type erasure
