# Generic Data Structures

Based on the codebase and documentation, future work for generics could include:

Forward Declarations or Prepass Collection

Allow calling a function before its definition by performing a prepass to record function signatures. This would remove the current order restriction.

Generic Constraints

The type_constraints.orus test assumes support for numeric comparisons and other operations. Adding a constraint system (e.g., T: Numeric) would provide compile-time checks for operations like < or +.

Improved Type Inference

While generics can be inferred from arguments, more advanced inference could reduce explicit type annotations—especially for nested generics or complex expressions.

Generic Arithmetic and Operators

The comprehensive test includes a specialized sum for [i32] because “fully generic arithmetic is not currently supported”:

Extending operator overloading or trait-based constraints would let arithmetic work generically across numeric types.

Better Collection and Iterator Support

The C-side macros (DEFINE_ARRAY_TYPE) provide generic arrays:

Building higher-level generic collections (maps, sets) and iterators would make generics more practical.

Error Reporting Enhancements

Diagnostics mention function-order issues but could also suggest using a forward declaration mechanism once implemented. Clearer messages for generic mismatches would aid debugging.

Cross-Module Generic Types

Current tests reside in single files. Future steps might include importing generic types from other modules and ensuring specialization works across compilation units.

Tooling and Documentation

Expand docs/GENERICS.md to include more complex patterns and best practices. Provide examples of advanced generics once new features land.

This roadmap aims to make generics more ergonomic and expressive, while also addressing the function-definition order constraint highlighted in the current design.