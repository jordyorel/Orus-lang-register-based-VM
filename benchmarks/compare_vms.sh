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
    echo "Running $name"
    ( time -p "$ORUS_EXEC" "$bench" > /dev/null ) 2>&1
    echo ""
done

