#!/bin/bash

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ORUS_EXECUTABLE="$(cd "$SCRIPT_DIR/.." && pwd)/orusc"

# Ensure interpreter exists
if [ ! -f "$ORUS_EXECUTABLE" ]; then
    (cd "$SCRIPT_DIR/.." && make > /dev/null)
fi

export ORUS_PATH="$(cd "$SCRIPT_DIR/.." && pwd)/std"

passed=0
failed=0

run_file_test() {
    local test_file=$1
    local category=$2
    local input_file="${test_file%.orus}.in"

    local stack_output
    local stack_exit
    local reg_output
    local reg_exit

    if [ -f "$input_file" ]; then
        stack_output=$("$ORUS_EXECUTABLE" "$test_file" < "$input_file" 2>&1)
        stack_exit=$?
        reg_output=$("$ORUS_EXECUTABLE" --regvm "$test_file" < "$input_file" 2>&1)
        reg_exit=$?
    else
        stack_output=$("$ORUS_EXECUTABLE" "$test_file" 2>&1)
        stack_exit=$?
        reg_output=$("$ORUS_EXECUTABLE" --regvm "$test_file" 2>&1)
        reg_exit=$?
    fi

    local name=$(basename "$test_file")

    if [ $stack_exit -ne $reg_exit ]; then
        echo -e "  ${RED}\u2717 FAIL${NC}: $category/$name (exit $stack_exit vs $reg_exit)"
        ((failed++))
        return
    fi

    if [ $stack_exit -eq 0 ] && [ "$stack_output" != "$reg_output" ]; then
        echo -e "  ${RED}\u2717 FAIL${NC}: $category/$name (output mismatch)"
        ((failed++))
        return
    fi

    echo -e "  ${GREEN}\u2713 PASS${NC}: $category/$name"
    ((passed++))
}

run_project_test() {
    local proj_dir=$1
    local category=$2

    local stack_output
    local stack_exit
    local reg_output
    local reg_exit

    stack_output=$("$ORUS_EXECUTABLE" --project "$proj_dir" 2>&1)
    stack_exit=$?
    reg_output=$("$ORUS_EXECUTABLE" --regvm --project "$proj_dir" 2>&1)
    reg_exit=$?

    local name=$(basename "$proj_dir")

    if [ $stack_exit -ne $reg_exit ]; then
        echo -e "  ${RED}\u2717 FAIL${NC}: $category/$name (exit $stack_exit vs $reg_exit)"
        ((failed++))
        return
    fi

    if [ $stack_exit -eq 0 ] && [ "$stack_output" != "$reg_output" ]; then
        echo -e "  ${RED}\u2717 FAIL${NC}: $category/$name (output mismatch)"
        ((failed++))
        return
    fi

    echo -e "  ${GREEN}\u2713 PASS${NC}: $category/$name"
    ((passed++))
}

echo -e "${YELLOW}====================================${NC}"
echo -e "${YELLOW}  Checking VM parity                ${NC}"
echo -e "${YELLOW}====================================${NC}"

for category_dir in $(find "$SCRIPT_DIR" -mindepth 1 -maxdepth 1 -type d | sort); do
    category="$(basename "$category_dir")"
    echo -e "${YELLOW}Running category: ${category}${NC}"
    if [ "$category" = "projects" ]; then
        for proj in "$category_dir"/*; do
            if [ -d "$proj" ] && [ -f "$proj/orus.json" ]; then
                run_project_test "$proj" "$category"
            fi
        done
    else
        for file in "$category_dir"/*.orus; do
            [ -f "$file" ] && run_file_test "$file" "$category"
        done
    fi
    echo ""
done

echo -e "${YELLOW}Parity summary:${NC} ${passed} passed, ${failed} failed"

if [ $failed -ne 0 ]; then
    exit 1
fi

