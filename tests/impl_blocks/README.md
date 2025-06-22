# Impl Blocks Test Suite

This directory contains comprehensive tests for the impl blocks functionality in the Orus programming language register VM. The tests verify that method calls, static methods, instance methods, and complex method interactions work correctly.

## Test Categories

### 1. Basic Method Calls (`basic_methods.orus`)
- **Purpose**: Tests fundamental impl block functionality with simple methods
- **Features Tested**:
  - Static methods (constructors)
  - Instance methods (getters/setters)
  - Methods with computations
  - Basic method call syntax

### 2. Static vs Instance Methods (`static_vs_instance.orus`)
- **Purpose**: Tests the difference between static and instance methods
- **Features Tested**:
  - Static methods called on types (`Type.method()`)
  - Instance methods called on objects (`object.method()`)
  - Methods with different parameter patterns
  - Return value handling

### 3. Method Chaining (`method_chaining.orus`)
- **Purpose**: Tests chaining method calls and fluent interfaces
- **Features Tested**:
  - Methods returning `self` for chaining
  - Complex method chains
  - State modification through chains
  - Mixed return types in chains

### 4. Mixed Types (`mixed_types.orus`)
- **Purpose**: Tests methods with different parameter and return types
- **Features Tested**:
  - String parameters and returns
  - Integer parameters and returns
  - Float parameters and returns
  - Boolean parameters and returns
  - Complex method logic with type mixing

### 5. Method Parameters (`method_parameters.orus`)
- **Purpose**: Tests methods with various parameter combinations
- **Features Tested**:
  - Methods with no parameters (except self)
  - Methods with single parameters
  - Methods with multiple parameters
  - Methods with mixed parameter types
  - Default parameter behavior

### 6. Nested Method Calls (`nested_method_calls.orus`)
- **Purpose**: Tests calling methods within methods and complex interactions
- **Features Tested**:
  - Methods calling other methods on same object
  - Methods calling methods on different objects
  - Complex computation chains
  - Object comparison through methods

### 7. Multiple Impl Blocks (`multiple_impl_blocks.orus`)
- **Purpose**: Tests having multiple impl blocks for the same struct
- **Features Tested**:
  - Methods defined across multiple impl blocks
  - Method availability from all blocks
  - Logical grouping of methods
  - Cross-block method interactions

### 8. Complex Interactions (`complex_interactions.orus`)
- **Purpose**: Tests complex scenarios with nested structs and advanced patterns
- **Features Tested**:
  - Nested struct method calls
  - Methods with struct parameters
  - Complex mathematical operations
  - Game object simulation patterns
  - Vector mathematics through methods

## Running the Tests

### Run All Tests
```bash
./run_impl_tests.sh
```

### Run Individual Test
```bash
orus basic_methods.orus
orus method_chaining.orus
# etc.
```

## Test Architecture

Each test follows a consistent pattern:

1. **Struct Definition**: Defines one or more structs with fields
2. **Impl Block(s)**: Implements methods for the structs
3. **Main Function**: Tests various method call scenarios
4. **Expected Output**: Comments showing expected results

## Register VM Features Tested

### Core Method Call Infrastructure
- `ROP_CALL_METHOD` opcode execution
- Method name resolution and dispatch
- `self` parameter handling in register allocation
- Method argument passing through registers

### Method Types
- **Static Methods**: No `self` parameter, called on type
- **Instance Methods**: Have `self` parameter, called on objects
- **Chainable Methods**: Return `self` or new objects for chaining
- **Utility Methods**: Helper functions within impl blocks

### Register VM Optimizations
- Register 0 protection for `self` parameter
- Efficient argument passing to method calls
- Proper register allocation for method parameters
- Method call frame management

## Error Cases Tested

The tests also verify proper error handling for:
- Invalid method calls
- Wrong parameter counts
- Type mismatches in method calls
- Bounds checking in methods
- Null/nil handling in methods

## Integration with Register VM

These tests specifically verify:
1. **Stack VM to Register VM Translation**: Method calls are properly translated from `OP_CALL` to `ROP_CALL_METHOD`
2. **Register Allocation**: Method parameters are correctly allocated to registers
3. **Method Dispatch**: Method names are properly resolved to function indices
4. **Frame Management**: Method calls create proper call frames
5. **Self Parameter Handling**: The `self` parameter is correctly passed as the first argument

## Performance Considerations

The tests also implicitly verify performance characteristics:
- Method calls should be efficient
- Register allocation should minimize spilling
- Method chains should not cause stack overflow
- Complex method interactions should execute in reasonable time

## Compatibility

These tests are designed to work with:
- Register VM execution
- Stack VM to Register VM translation
- Future direct register VM compilation
- Both debug and optimized builds

## Expected Results

All tests should pass with the register VM implementation. The tests produce specific output that can be verified for correctness. Any failures indicate issues with:
- Method call translation
- Register allocation
- Method dispatch
- Parameter passing
- Frame management

## Roadmap Integration

These tests fulfill **Phase 1.2.3** of the Register VM Features Roadmap:
- ✅ Test basic impl block methods
- ✅ Test method chaining  
- ✅ Test static vs instance methods
- ✅ Test complex method interactions
- ✅ Test multiple impl blocks
- ✅ Test nested method calls
- ✅ Test various parameter types
- ✅ Comprehensive error case coverage