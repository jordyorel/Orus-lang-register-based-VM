#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ORUS_EXEC="$SCRIPT_DIR/../orusc"

if [ ! -f "$ORUS_EXEC" ]; then
    (cd "$SCRIPT_DIR/.." && make)
fi

echo "Using interpreter: $ORUS_EXEC"

echo ""
for bench in "$SCRIPT_DIR"/*.orus; do
    [ -f "$bench" ] || continue
    name=$(basename "$bench")
    echo "Running $name (stack VM)"
    ( time -p "$ORUS_EXEC" "$bench" > /dev/null ) 2>&1
    echo "Running $name (register VM)"
    output=$( ( time -p "$ORUS_EXEC" --regvm "$bench" > /dev/null ) 2>&1 )
    status=$?
    if [ $status -eq 0 ]; then
        echo "$output"
    else
        echo "Register VM failed with status $status"
    fi
    echo ""
done

