# Generic Tests

This directory contains comprehensive test cases for generic support in the Orus register VM.

## Test Categories

### Working Generic Tests
- **`simple_generic.orus`**: Basic generic function with identity
- **`working_generic.orus`**: Multiple generic functions with different types
- **`manual_generic_test.orus`**: Manual generic simulation using existing features

### Advanced Generic Tests (May Need Parser Work)
- **`basic_generic_function.orus`**: Generic functions with constraints (fails on comparison)
- **`generic_struct.orus`**: Generic struct definitions (syntax may not be supported)
- **`generic_constraints.orus`**: Generic type constraints (complex constraints)
- **`generic_array.orus`**: Generic higher-order functions (complex syntax)

## Running Tests

To run all generic tests:
```bash
./run_generic_tests.sh
```

To run individual tests:
```bash
../../orusc simple_generic.orus
../../orusc working_generic.orus
# etc.
```

## Current Status

### ✅ WORKING FEATURES
1. **Basic Generic Functions**: `fn identity<T>(value: T) -> T`
2. **Type Instantiation**: `identity<i32>(42)`, `identity<string>("hello")`
3. **Multiple Type Parameters**: Functions with different generic types
4. **Register VM Integration**: `ROP_CALL_GENERIC` opcode implemented
5. **Type System**: Complete generic type representation and resolution

### 🚧 PARTIAL SUPPORT
1. **Type Constraints**: Some constraint checking implemented but needs refinement
2. **Complex Operations**: Operations like `>`, `<` require Comparable constraint

### ❌ NOT YET SUPPORTED
1. **Generic Structs**: `struct Container<T> { value: T }`
2. **Generic Enums**: `enum Option<T> { Some(T), None }`
3. **Complex Constraints**: `T: Numeric + Comparable`
4. **Type Inference**: Automatic type parameter deduction

## Implementation Status

✅ **CORE INFRASTRUCTURE COMPLETE**
- Generic type representation (`TYPE_GENERIC`)
- Type substitution (`substituteGenerics`)
- Generic function calls (`ROP_CALL_GENERIC`)
- AST support for generic parameters
- Register VM integration

✅ **BASIC FUNCTIONALITY WORKING**
- Simple generic functions compile and run correctly
- Type instantiation with explicit type parameters
- Multiple generic type parameters supported

🎯 **NEXT STEPS**
1. Improve constraint handling for comparison operations
2. Add generic struct support to parser
3. Implement generic type inference
4. Add support for generic enums

## Technical Details

The generic system uses:
- **`Type` system**: `TYPE_GENERIC` with name-based resolution
- **AST integration**: Generic parameters in function and struct definitions
- **Compiler support**: `deduceGenerics` and `substituteGenerics` functions
- **Register VM**: `ROP_CALL_GENERIC` opcode for generic function calls
- **Memory management**: Proper generic type allocation and cleanup

## Test Results

✅ Basic generic functions work perfectly
✅ Type instantiation functional
✅ Register VM integration complete
⚠️ Advanced features need parser/constraint work