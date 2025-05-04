#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ORUS_EXECUTABLE="../orus"

# Function to run tests in a category
run_category_tests() {
    local category=$1
    echo -e "${YELLOW}Running tests for category: ${category}${NC}"
    
    # Find all .orus files in the category directory
    for test_file in "${SCRIPT_DIR}/${category}"/*.orus; do
        if [ -f "$test_file" ]; then
            test_name=$(basename "$test_file")
            echo -e "  Testing: ${test_name}..."
            
            # Run the test and capture the output
            output=$($ORUS_EXECUTABLE "$test_file" 2>&1)
            exit_code=$?
            
            if [ $exit_code -eq 0 ]; then
                echo -e "  ${GREEN}✓ PASS${NC}: $test_name"
            else
                echo -e "  ${RED}✗ FAIL${NC}: $test_name"
                # Show the error message
                echo -e "  Error details:"
                echo -e "  $output" | sed 's/^/  /'
            fi
        fi
    done
    echo ""
}

# Main function to run all tests
run_all_tests() {
    echo -e "${YELLOW}====================================${NC}"
    echo -e "${YELLOW}   Running Orus Language Tests      ${NC}"
    echo -e "${YELLOW}====================================${NC}"
    
    # Get all category directories
    categories=()
    for dir in "$SCRIPT_DIR"/*/; do
        if [ -d "$dir" ]; then
            category=$(basename "$dir")
            categories+=("$category")
        fi
    done
    
    # Sort categories
    IFS=$'\n' sorted_categories=($(sort <<<"${categories[*]}"))
    unset IFS
    
    # Run tests for each category
    for category in "${sorted_categories[@]}"; do
        run_category_tests "$category"
    done
    
    echo -e "${YELLOW}====================================${NC}"
    echo -e "${YELLOW}   All tests completed              ${NC}"
    echo -e "${YELLOW}====================================${NC}"
}

# Call the main function
run_all_tests
