#!/bin/bash
./bin/function_injector \
    tests/data/test_edge_cases.c \
    -m printf \
    -i auto
