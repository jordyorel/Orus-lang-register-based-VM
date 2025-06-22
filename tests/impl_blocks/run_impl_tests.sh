#!/bin/bash

# Comprehensive Impl Blocks Test Runner
# Tests all impl block functionality in the register VM

set -e  # Exit on any error

echo "======================================="
echo "  Orus Impl Blocks Test Suite"
echo "======================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ORUS_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Build the interpreter if needed
if [[ ! -f "$ORUS_ROOT/orus" ]]; then
    echo -e "${YELLOW}Building Orus interpreter...${NC}"
    cd "$ORUS_ROOT"
    make clean && make
    cd "$SCRIPT_DIR"
fi

# Test cases with descriptions
declare -A tests=(
    ["basic_methods.orus"]="Basic method calls (getters, setters, computations)"
    ["static_vs_instance.orus"]="Static methods vs instance methods"
    ["method_chaining.orus"]="Method chaining and fluent interfaces"
    ["mixed_types.orus"]="Methods with different parameter and return types"
    ["method_parameters.orus"]="Various parameter combinations and types"
    ["nested_method_calls.orus"]="Methods calling other methods"
    ["multiple_impl_blocks.orus"]="Multiple impl blocks for same struct"
    ["complex_interactions.orus"]="Complex struct interactions and nested calls"
)

# Counters
total_tests=0
passed_tests=0
failed_tests=0

echo "Running impl block tests..."
echo ""

# Run each test
for test_file in "${!tests[@]}"; do
    description="${tests[$test_file]}"
    total_tests=$((total_tests + 1))
    
    echo -e "${YELLOW}Test $total_tests: $test_file${NC}"
    echo "Description: $description"
    echo "----------------------------------------"
    
    if [[ -f "$SCRIPT_DIR/$test_file" ]]; then
        # Run the test with timeout
        if timeout 10s "$ORUS_ROOT/orus" "$SCRIPT_DIR/$test_file" 2>&1; then
            echo -e "${GREEN}✓ PASSED${NC}"
            passed_tests=$((passed_tests + 1))
        else
            echo -e "${RED}✗ FAILED${NC}"
            failed_tests=$((failed_tests + 1))
        fi
    else
        echo -e "${RED}✗ TEST FILE NOT FOUND${NC}"
        failed_tests=$((failed_tests + 1))
    fi
    
    echo ""
done

# Summary
echo "======================================="
echo "  Test Results Summary"
echo "======================================="
echo "Total tests: $total_tests"
echo -e "Passed: ${GREEN}$passed_tests${NC}"
echo -e "Failed: ${RED}$failed_tests${NC}"

if [[ $failed_tests -eq 0 ]]; then
    echo -e "${GREEN}All tests passed! 🎉${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. 😞${NC}"
    exit 1
fi