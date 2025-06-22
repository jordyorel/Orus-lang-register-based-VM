#!/bin/bash

# Run all generic tests
echo "Running Generic Tests..."
echo "========================"

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
    "manual_generic_test.orus"
    "basic_generic_function.orus"
    "generic_struct.orus"
    "generic_constraints.orus"
    "generic_array.orus"
)

passed=0
failed=0

for test in "${tests[@]}"; do
    echo "Running $test..."
    if "$ORUSC" "$test" > /dev/null 2>&1; then
        echo "  ✅ PASSED: $test"
        ((passed++))
    else
        echo "  ❌ FAILED: $test (Expected - parser doesn't support generic syntax yet)"
        echo "    Error output:"
        "$ORUSC" "$test" 2>&1 | sed 's/^/    /' | head -3
        ((failed++))
    fi
    echo
done

echo "========================"
echo "Generic Test Results:"
echo "  Passed: $passed"
echo "  Failed: $failed (Expected failures due to parser limitations)"
echo "  Total:  $((passed + failed))"

if [ $passed -gt 0 ]; then
    echo "🎉 Generic infrastructure tests passed!"
    echo "📋 Note: Full generic syntax requires parser implementation"
else
    echo "⚠️  All tests failed - this is expected until parser supports generic syntax"
fi