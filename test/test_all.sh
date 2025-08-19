#!/usr/bin/env bash
set -euo pipefail

# List of test source files (without .c)
tests=(
  test_version_node
  test_hash
  test_field
  test_document
  test_database
)

make all

for t in "${tests[@]}"; do
  echo "Running $t..."
  ./"$t"
  echo
done

echo "All tests completed successfully."

