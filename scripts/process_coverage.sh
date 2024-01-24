#!/bin/bash

set -e

GCOV_DIR="build/gcov/out"
JSON_DIR="${GCOV_DIR}/json"

# Arg 1 is expected to have this format: "test/test_u_cx_at_utils.c"
TEST_FILE=$(basename -- "$1")
# Remove .c extension
TEST_NAME=${TEST_FILE%%.*}

mkdir -p ${JSON_DIR}

gcovr --json ${JSON_DIR}/${TEST_NAME}.json --filter src/ --delete ${GCOV_DIR}
