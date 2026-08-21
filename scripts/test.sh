#!/usr/bin/env bash
set -e

BUILD_DIR="build"

echo "==> Running Unit Tests via CTest..."
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "==> Running Parser Fuzz Tester..."
./"$BUILD_DIR"/mininginx_fuzz_parser 5000

echo "==> Running Live Integration Tests..."
python3 tests/integration/test_server_integration.py

echo "==> All Tests Passed!"
