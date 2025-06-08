#!/bin/bash
set -e
# Build all test executables
make recurrence_tests event_tests model_tests model_comprehensive_tests controller_tests view_tests api_tests database_tests

# Run each test executable sequentially
for t in recurrence_tests event_tests model_tests model_comprehensive_tests controller_tests view_tests api_tests database_tests; do
    echo "Running $t"
    ./$t
    echo
done
