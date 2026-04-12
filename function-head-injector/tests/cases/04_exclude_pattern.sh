#!/bin/bash
./bin/function_injector \
    tests/data/test_simple.c \
    tests/data/injection_code.txt \
    -e tests/data/exclude_pattern.txt
