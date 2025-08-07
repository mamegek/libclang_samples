#!/bin/bash

##################################
# Build function_injector        #
##################################
make

printf "\n-------------------------------------\n"
##################################
# Usage of function_injector     #
##################################
./bin/function_injector

printf "\n-------------------------------------\n"
##################################
# Run function_injector (simple) #
##################################
./bin/function_injector \
    test_input/test_simple.c \
    test_input/injection_code.txt

printf "\n-------------------------------------\n"
##################################
# Run function_injector (full)   #
##################################
# Create temporary file
TMPFILE=$(mktemp /tmp/function_injector_generated.XXXXXX.c)

./bin/function_injector \
    test_input/test_simple.c \
    test_input/injection_code.txt \
    -e test_input/exclude_pattern.txt \
    -o "$TMPFILE"

# Check if the command succeeded
if [ $? -eq 0 ]; then
    printf "Generated program to: %s\n" "$TMPFILE"
    printf "Please check the output file above.\n"
fi