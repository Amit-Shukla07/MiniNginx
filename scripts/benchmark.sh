#!/usr/bin/env bash
set -e

PORT=8888
BIN="./build/mininginx"

echo "==> Starting MiniNginx server on port $PORT..."
$BIN -p $PORT -w 4 &
SERVER_PID=$!

cleanup() {
    echo "==> Stopping MiniNginx (PID: $SERVER_PID)..."
    kill -INT $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}
trap cleanup EXIT

# Wait for server to accept connections
sleep 1

echo "==> Running Benchmark Suite across Concurrency Levels..."
python3 benchmarks/benchmark_server.py --port $PORT --path /api/health -c 10 -n 1000
python3 benchmarks/benchmark_server.py --port $PORT --path /api/health -c 50 -n 2500
python3 benchmarks/benchmark_server.py --port $PORT --path /api/health -c 100 -n 5000

echo "==> Benchmark Completed!"
