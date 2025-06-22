#!/bin/bash

# Array test runner for the new array implementation
# Runs all array test suites and reports results

echo "🚀 ORUS ARRAY IMPLEMENTATION TEST SUITE"
echo "========================================"
echo "Testing the new modern array implementation"
echo ""

# Get the script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ORUS_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
ORUSC="$ORUS_ROOT/orusc"

# Check if compiler exists
if [[ ! -f "$ORUSC" ]]; then
    echo "❌ Error: Orus compiler not found at $ORUSC"
    echo "Please run 'make' to build the compiler first"
    exit 1
fi

echo "Using compiler: $ORUSC"
echo "Test directory: $SCRIPT_DIR"
echo ""

# Array to track test results
declare -a test_results=()
total_tests=0
passed_tests=0

# Function to run a test
run_test() {
    local test_file="$1"
    local test_name="$2"
    
    echo "▶️  Running $test_name..."
    echo "   File: $test_file"
    
    total_tests=$((total_tests + 1))
    
    # Run the test and capture output and exit code
    if output=$("$ORUSC" "$test_file" 2>&1); then
        echo "✅ PASSED: $test_name"
        test_results+=("✅ PASSED: $test_name")
        passed_tests=$((passed_tests + 1))
        
        # Show key output lines
        echo "   Output preview:"
        echo "$output" | head -5 | sed 's/^/   │ /'
        if [[ $(echo "$output" | wc -l) -gt 5 ]]; then
            echo "   │ ... (truncated)"
        fi
    else
        echo "❌ FAILED: $test_name"
        test_results+=("❌ FAILED: $test_name")
        
        # Show error output
        echo "   Error output:"
        echo "$output" | sed 's/^/   │ /'
    fi
    
    echo ""
}

# Run all array tests
echo "Running array test suite..."
echo ""

# Test 1: Basic operations
if [[ -f "$SCRIPT_DIR/test_basic_array_ops.orus" ]]; then
    run_test "$SCRIPT_DIR/test_basic_array_ops.orus" "Basic Array Operations"
fi

# Test 2: Register VM opcodes
if [[ -f "$SCRIPT_DIR/test_register_vm_opcodes.orus" ]]; then
    run_test "$SCRIPT_DIR/test_register_vm_opcodes.orus" "Register VM Opcodes"
fi

# Test 3: Performance tests
if [[ -f "$SCRIPT_DIR/test_array_performance.orus" ]]; then
    run_test "$SCRIPT_DIR/test_array_performance.orus" "Array Performance"
fi

# Test 4: Error handling
if [[ -f "$SCRIPT_DIR/test_array_errors.orus" ]]; then
    run_test "$SCRIPT_DIR/test_array_errors.orus" "Error Handling"
fi

# Test 5: Comprehensive test
if [[ -f "$SCRIPT_DIR/test_new_array_implementation.orus" ]]; then
    run_test "$SCRIPT_DIR/test_new_array_implementation.orus" "Comprehensive Array Tests"
fi

# Test 6: Run existing array tests to check compatibility
echo "▶️  Running existing array tests for compatibility..."
echo ""

# Find and run existing array tests
existing_array_tests=(
    "$ORUS_ROOT/tests/types/array_literal.orus"
    "$ORUS_ROOT/tests/types/array_operations.orus"
    "$ORUS_ROOT/tests/types/array_indexing.orus"
    "$ORUS_ROOT/tests/types/dynamic_array.orus"
)

for test_file in "${existing_array_tests[@]}"; do
    if [[ -f "$test_file" ]]; then
        test_name="Legacy: $(basename "$test_file" .orus)"
        run_test "$test_file" "$test_name"
    fi
done

# Print summary
echo "========================================"
echo "🏁 TEST SUMMARY"
echo "========================================"
echo "Total tests run: $total_tests"
echo "Passed: $passed_tests"
echo "Failed: $((total_tests - passed_tests))"
echo ""

# Print all results
echo "📋 DETAILED RESULTS:"
for result in "${test_results[@]}"; do
    echo "$result"
done

echo ""

# Final status
if [[ $passed_tests -eq $total_tests ]]; then
    echo "🎉 ALL TESTS PASSED! Array implementation is working correctly!"
    exit 0
elif [[ $passed_tests -gt 0 ]]; then
    echo "⚠️  PARTIAL SUCCESS: $passed_tests/$total_tests tests passed"
    echo "   Some functionality may not be fully integrated yet"
    exit 1
else
    echo "❌ ALL TESTS FAILED: Array implementation needs debugging"
    exit 1
fi