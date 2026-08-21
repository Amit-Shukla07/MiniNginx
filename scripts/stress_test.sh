#!/usr/bin/env bash
set -e

PORT=8765
BIN="./build/mininginx"

echo "==> Launching MiniNginx for stress testing on port $PORT..."
$BIN -p $PORT -w 4 &
SERVER_PID=$!

cleanup() {
    echo "==> Shutting down MiniNginx..."
    kill -INT $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}
trap cleanup EXIT

sleep 1
python3 tests/stress/stress_test.py 127.0.0.1 $PORT
