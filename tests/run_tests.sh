#!/bin/bash
# tests/run_tests.sh

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

run_test() {
    local test_file=$1
    local expected_output=$2
    local expected_error=$3
    
    echo "Running test: $test_file"
    
    # Run the test and capture both stdout and stderr
    output=$(../clox "$test_file" 2>&1)
    exit_code=$?
    
    # Filter out debug messages and keep only actual output
    filtered_output=$(echo "$output" | grep -v "^DEBUG:" | 
                                     grep -v "^==" | 
                                     grep -v "^\[" | 
                                     grep -v "^[0-9][0-9][0-9][0-9]" |
                                     grep -v "^$" |
                                     grep -v "chunk to execute" |
                                     grep -v "Running chunk" |
                                     sed '/^$/d')
    
    if [ -n "$expected_error" ]; then
        # Error test case
        if echo "$output" | grep -q "$expected_error"; then
            echo -e "${GREEN}PASS${NC}: Expected error found"
        else
            echo -e "${RED}FAIL${NC}: Expected error '$expected_error' not found"
            echo "Got: $output"
        fi
    else
        # Success test case
        if [ "$filtered_output" = "$expected_output" ]; then
            echo -e "${GREEN}PASS${NC}"
        else
            echo -e "${RED}FAIL${NC}"
            echo "Expected:"
            echo "$expected_output"
            echo "Got:"
            echo "$filtered_output"
            echo "Raw output:"
            echo "$output"
        fi
    fi
    echo
}

# Run the tests
run_test "basic/literals.orus" "42
3.14
true
false"

run_test "basic/variables.orus" "10
10"

run_test "types/annotations.orus" "42
42
42
true"

run_test "types/coercion.orus" "42
42"

run_test "errors/type_errors.orus" "" "Type mismatch"