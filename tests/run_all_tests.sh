#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Fix the path to the Orus executable
ORUS_EXECUTABLE="$(cd "$SCRIPT_DIR/.." && pwd)/orus"

# Overall counters
passed=0
failed=0

# Function to run tests in a category
run_category_tests() {
    local category=$1
    echo -e "${YELLOW}Running tests for category: ${category}${NC}"

    # Initialize counter for this category
    local category_pass=0
    local category_fail=0
    
    # Find all .orus files in the category directory
    for test_file in "${SCRIPT_DIR}/${category}"/*.orus; do
        if [ -f "$test_file" ]; then
            test_name=$(basename "$test_file")
            echo -e "  Testing: ${test_name}..."
            
            # Check if the test has an accompanying .in file to provide stdin
            input_file="${test_file%.orus}.in"
            if [ -f "$input_file" ]; then
                output=$($ORUS_EXECUTABLE "$test_file" < "$input_file" 2>&1)
            else
                output=$($ORUS_EXECUTABLE "$test_file" 2>&1)
            fi
            exit_code=$?

            # Determine expected behaviour for error tests
            if [[ "$category" = "errors" || "$category" == */errors ]]; then
                if [ $exit_code -ne 0 ]; then
                    echo -e "  ${GREEN}✓ PASS${NC}: $test_name"
                    ((category_pass++))
                    ((passed++))
                else
                    echo -e "  ${RED}✗ FAIL${NC}: $test_name"
                    echo -e "  Error details:"
                    echo -e "  $output" | sed 's/^/  /'
                    ((category_fail++))
                    ((failed++))
                fi
            else
                if [ $exit_code -eq 0 ]; then
                    echo -e "  ${GREEN}✓ PASS${NC}: $test_name"
                    ((category_pass++))
                    ((passed++))
                else
                    echo -e "  ${RED}✗ FAIL${NC}: $test_name"
                    # Show the error message
                    echo -e "  Error details:"
                    echo -e "  $output" | sed 's/^/  /'
                    ((category_fail++))
                    ((failed++))
                fi
            fi
        fi
    done
    echo -e "  ${YELLOW}Category summary:${NC} ${category_pass} passed, ${category_fail} failed"
    echo ""
}

# Main function to run all tests
run_all_tests() {
    echo -e "${YELLOW}====================================${NC}"
    echo -e "${YELLOW}   Running Orus Language Tests      ${NC}"
    echo -e "${YELLOW}====================================${NC}"
    
    # Get all category directories
    categories=()
    while IFS= read -r -d '' dir; do
        category="${dir#$SCRIPT_DIR/}"
        categories+=("$category")
    done < <(find "$SCRIPT_DIR" -mindepth 1 -type d -print0)
    
    # Sort categories
    IFS=$'\n' sorted_categories=($(sort <<<"${categories[*]}"))
    unset IFS
    
    # Run tests for each category
    for category in "${sorted_categories[@]}"; do
        run_category_tests "$category"
    done

    echo -e "${YELLOW}Test summary:${NC} ${passed} passed, ${failed} failed"

    echo -e "${YELLOW}====================================${NC}"
    echo -e "${YELLOW}   All tests completed              ${NC}"
    echo -e "${YELLOW}====================================${NC}"

    if [ $failed -ne 0 ]; then
        return 1
    fi
}

# Call the main function
run_all_tests
