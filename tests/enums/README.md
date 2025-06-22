# Enum Tests

This directory contains comprehensive test cases for enum support in the Orus register VM.

## Test Categories

### Basic Enum Tests
- **`basic_enum.orus`**: Simple unit variants (Color::Red, Color::Green, Color::Blue)
- **`enum_with_data.orus`**: Variants with associated data (Shape::Circle(radius))

### Pattern Matching Tests  
- **`pattern_matching.orus`**: Match expressions with enum variants
- **`complex_enum.orus`**: Complex pattern matching with multiple data types

### Advanced Features
- **`enum_functions.orus`**: Enums as function parameters and return values
- **`nested_enum.orus`**: Enums containing other enums

## Running Tests

To run all enum tests:
```bash
./run_enum_tests.sh
```

To run individual tests:
```bash
../../orusc basic_enum.orus
../../orusc enum_with_data.orus
# etc.
```

## Expected Features Tested

1. **Enum Declaration**: `enum Color { Red, Green, Blue }`
2. **Unit Variants**: `Color.Red` (no associated data) - **DOT NOTATION**
3. **Data Variants**: `Shape.Circle(5.0)` (with associated data) - **DOT NOTATION**
4. **Pattern Matching**: `match value { Pattern => action }`
5. **Variant Checking**: `if value == Color.Red`
6. **Function Integration**: Enums as parameters and return values
7. **Nested Enums**: Enums containing other enum values
8. **Memory Management**: Proper allocation and cleanup of enum data

### Syntax Choice: Dot Notation
This test suite uses **dot notation** (`Color.Red`) instead of double colon notation (`Color::Red`) for accessing enum variants, making the syntax more intuitive and easier to use.

## Implementation Status

- ✅ Enum opcodes defined (ROP_ENUM_LITERAL, ROP_ENUM_VARIANT, ROP_ENUM_CHECK)
- ✅ Register VM implementation completed
- ✅ Memory management (allocation, GC, cleanup)
- ✅ Stack VM to Register VM translation
- ✅ Debug support for enum opcodes
- 🚧 Parser support (may need implementation)
- 🚧 Type system integration (may need implementation)
- 🚧 Match expression parsing (may need implementation)

## Notes

These tests assume the Orus language syntax for enums and pattern matching. If the parser doesn't yet support enum syntax, these tests will fail at the parsing stage, not at the register VM level. The register VM implementation is complete and ready to execute enum operations once the parser support is added.