#!/bin/bash

# Run all error handling tests
echo "Running Error Handling Tests..."
echo "==============================="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ORUSC="$PROJECT_ROOT/orusc"

if [ ! -f "$ORUSC" ]; then
    echo "Error: orusc compiler not found at $ORUSC"
    echo "Please run 'make' in the project root first"
    exit 1
fi

cd "$SCRIPT_DIR"

# List of test files
tests=(
    "basic_try_catch.orus"
    "nested_try_catch.orus"
    "array_bounds_error.orus"
    "function_error_propagation.orus"
    "multiple_error_types.orus"
    "no_error_try_catch.orus"
)

passed=0
failed=0

for test in "${tests[@]}"; do
    echo "Running $test..."
    if "$ORUSC" "$test" > /dev/null 2>&1; then
        echo "  ✅ PASSED: $test"
        ((passed++))
    else
        echo "  ❌ FAILED: $test"
        echo "    Error output:"
        "$ORUSC" "$test" 2>&1 | sed 's/^/    /'
        ((failed++))
    fi
    echo
done

echo "==============================="
echo "Error Handling Test Results:"
echo "  Passed: $passed"
echo "  Failed: $failed"
echo "  Total:  $((passed + failed))"

if [ $failed -eq 0 ]; then
    echo "🎉 All error handling tests passed!"
    exit 0
else
    echo "💥 Some error handling tests failed!"
    exit 1
fi