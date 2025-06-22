#!/bin/bash

# Run all enum tests
echo "Running Enum Tests..."
echo "====================="

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
    "basic_enum.orus"
    "enum_with_data.orus"
    "pattern_matching.orus"
    "complex_enum.orus"
    "enum_functions.orus"
    "nested_enum.orus"
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

echo "====================="
echo "Enum Test Results:"
echo "  Passed: $passed"
echo "  Failed: $failed"
echo "  Total:  $((passed + failed))"

if [ $failed -eq 0 ]; then
    echo "🎉 All enum tests passed!"
    exit 0
else
    echo "💥 Some enum tests failed!"
    exit 1
fi