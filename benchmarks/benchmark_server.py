#!/usr/bin/env python3
import argparse
import http.client
import json
import os
import statistics
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor

def worker_task(host, port, path, requests_per_thread, latencies, errors, keep_alive=True):
    completed = 0
    while completed < requests_per_thread:
        try:
            conn = http.client.HTTPConnection(host, port, timeout=5)
            batch = min(50, requests_per_thread - completed) if keep_alive else 1
            for _ in range(batch):
                start = time.perf_counter()
                headers = {"Connection": "keep-alive" if keep_alive else "close"}
                conn.request("GET", path, headers=headers)
                res = conn.getresponse()
                res.read()
                elapsed_ms = (time.perf_counter() - start) * 1000.0
                if res.status == 200:
                    latencies.append(elapsed_ms)
                else:
                    errors.append(1)
                completed += 1
            conn.close()
        except Exception:
            errors.append(1)
            completed += 1

def run_benchmark(host, port, path, concurrency, total_requests, keep_alive=True):
    print(f"\n=======================================================")
    print(f"Benchmarking MiniNginx: {concurrency} clients | {total_requests} requests (Keep-Alive: {keep_alive})")
    print(f"Target: http://{host}:{port}{path}")
    print(f"=======================================================")

    requests_per_thread = total_requests // concurrency
    latencies = []
    errors = []

    start_time = time.perf_counter()

    with ThreadPoolExecutor(max_workers=concurrency) as executor:
        futures = [
            executor.submit(worker_task, host, port, path, requests_per_thread, latencies, errors, keep_alive)
            for _ in range(concurrency)
        ]
        for f in futures:
            f.result()

    total_time = time.perf_counter() - start_time
    completed_requests = len(latencies)
    rps = completed_requests / total_time if total_time > 0 else 0

    latencies.sort()
    avg_lat = statistics.mean(latencies) if latencies else 0
    p50 = latencies[int(len(latencies) * 0.50)] if latencies else 0
    p90 = latencies[int(len(latencies) * 0.90)] if latencies else 0
    p95 = latencies[int(len(latencies) * 0.95)] if latencies else 0
    p99 = latencies[int(len(latencies) * 0.99)] if latencies else 0

    print(f"Total Time:         {total_time:.3f} s")
    print(f"Completed Requests: {completed_requests}")
    print(f"Failed Requests:    {len(errors)}")
    print(f"Requests / Second:  {rps:.2f} req/s")
    print(f"Latency Mean:       {avg_lat:.2f} ms")
    print(f"Latency P50:        {p50:.2f} ms")
    print(f"Latency P90:        {p90:.2f} ms")
    print(f"Latency P95:        {p95:.2f} ms")
    print(f"Latency P99:        {p99:.2f} ms")
    print(f"=======================================================\n")

    # Save results to benchmarks/results/
    os.makedirs("benchmarks/results", exist_ok=True)
    res_data = {
        "concurrency": concurrency,
        "total_requests": total_requests,
        "keep_alive": keep_alive,
        "duration_sec": round(total_time, 4),
        "requests_per_sec": round(rps, 2),
        "latency_mean_ms": round(avg_lat, 2),
        "latency_p50_ms": round(p50, 2),
        "latency_p90_ms": round(p90, 2),
        "latency_p95_ms": round(p95, 2),
        "latency_p99_ms": round(p99, 2),
        "errors": len(errors)
    }
    with open(f"benchmarks/results/benchmark_c{concurrency}.json", "w") as f:
        json.dump(res_data, f, indent=2)

    return res_data

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MiniNginx Benchmarking Client")
    parser.add_argument("--host", default="127.0.0.1", help="Server host")
    parser.add_argument("--port", type=int, default=8080, help="Server port")
    parser.add_argument("--path", default="/api/health", help="HTTP request path")
    parser.add_argument("-c", "--concurrency", type=int, default=50, help="Concurrent client threads")
    parser.add_argument("-n", "--requests", type=int, default=2000, help="Total requests")
    parser.add_argument("--no-keep-alive", action="store_true", help="Disable persistent keep-alive")
    args = parser.parse_args()

    run_benchmark(args.host, args.port, args.path, args.concurrency, args.requests, keep_alive=not args.no_keep_alive)
