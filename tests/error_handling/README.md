# Error Handling Tests

This directory contains comprehensive test cases for error handling and try-catch functionality in the Orus register VM.

## Test Categories

### Basic Error Handling
- **`basic_try_catch.orus`**: Simple try-catch with division by zero error
- **`no_error_try_catch.orus`**: Try-catch block with no errors (normal execution)

### Advanced Error Scenarios
- **`nested_try_catch.orus`**: Nested try-catch blocks with multiple error levels
- **`function_error_propagation.orus`**: Error propagation through function calls
- **`multiple_error_types.orus`**: Different types of runtime errors

### Specific Error Types
- **`array_bounds_error.orus`**: Array index out of bounds error handling

## Running Tests

To run all error handling tests:
```bash
./run_error_tests.sh
```

To run individual tests:
```bash
../../orusc basic_try_catch.orus
../../orusc nested_try_catch.orus
# etc.
```

## Expected Features Tested

1. **Try-Catch Syntax**: `try { ... } catch error { ... }`
2. **Error Propagation**: Errors bubble up through function calls
3. **Error Types**: Division by zero, array bounds, modulo by zero
4. **Nested Exception Handling**: Multiple levels of try-catch blocks
5. **Error Value Access**: Accessing error information in catch blocks
6. **Normal Execution**: Try blocks that complete without errors

## Implementation Status

✅ **FULLY IMPLEMENTED AND TESTED**
- Exception opcodes (`ROP_SETUP_EXCEPT`, `ROP_POP_EXCEPT`)
- Exception stack management (`TryFrame` array)
- Error propagation through register VM dispatch loop
- Error value creation and passing
- Try-catch block parsing and execution
- Multiple error types supported
- Nested try-catch functionality

## Technical Details

The error handling system uses:
- **`TryFrame` structure**: Stores exception handler information
- **`vm.tryFrames[]` array**: Stack of active exception handlers
- **`vm.lastError`**: Current error value being handled
- **`ROP_SETUP_EXCEPT`**: Sets up exception handler
- **`ROP_POP_EXCEPT`**: Removes exception handler
- **Automatic error propagation**: Errors bubble up until caught

## Test Results

All error handling features are production-ready and working correctly in the register VM. The implementation provides robust exception handling with proper cleanup and error propagation.