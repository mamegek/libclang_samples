#!/bin/bash

# Change directory to the project root
cd "$(dirname "$0")/.." || exit 1

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Ensure binary exists
if [ ! -f "bin/function_injector" ]; then
    echo "Executable not found. Running make..."
    make
fi

# Flag to update expected outputs
UPDATE_SNAPSHOTS=0
if [ "$1" == "--update" ]; then
    UPDATE_SNAPSHOTS=1
    echo "Running in update mode: will overwrite expected files."
fi

# Directory for saving diff files and failed outputs.
# Only remove files we know we generate (.actual.c and .diff) instead of
# rm -rf on the whole directory, to avoid accidents if FAIL_DIR is misconfigured.
FAIL_DIR="tests/failures"
mkdir -p "$FAIL_DIR"
rm -f "${FAIL_DIR}"/*.actual.c "${FAIL_DIR}"/*.diff 2>/dev/null

# Track pass/fail
PASS_COUNT=0
FAIL_COUNT=0

mkdir -p tests/expected

echo "Starting tests..."
echo "-----------------------------------"

for test_script in tests/cases/*.sh; do
    [ -e "$test_script" ] || continue

    case_name=$(basename "$test_script" .sh)
    expected_file="tests/expected/${case_name}.c"
    actual_file="${FAIL_DIR}/${case_name}.actual.c"
    diff_file="${FAIL_DIR}/${case_name}.diff"

    # Run the test case, saving output to a persistent location
    bash "$test_script" > "$actual_file" 2>/dev/null

    if [ $UPDATE_SNAPSHOTS -eq 1 ]; then
        cp "$actual_file" "$expected_file"
        rm -f "$diff_file"
        echo -e "${GREEN}[UPDATED]${NC} $case_name  →  $expected_file"
    else
        if [ ! -f "$expected_file" ]; then
            echo -e "${RED}[FAIL]${NC} $case_name (expected file not found; run with --update)"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        elif diff -u "$expected_file" "$actual_file" > "$diff_file"; then
            echo -e "${GREEN}[PASS]${NC} $case_name"
            rm -f "$actual_file" "$diff_file"   # clean up on success only
            PASS_COUNT=$((PASS_COUNT + 1))
        else
            echo -e "${RED}[FAIL]${NC} $case_name"
            echo "  actual : $actual_file"
            echo "  diff   : $diff_file"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    fi
done

echo "-----------------------------------"
if [ $UPDATE_SNAPSHOTS -eq 0 ]; then
    echo "Results: $PASS_COUNT passed, $FAIL_COUNT failed."
    if [ $FAIL_COUNT -gt 0 ]; then
        exit 1
    fi
fi
exit 0
