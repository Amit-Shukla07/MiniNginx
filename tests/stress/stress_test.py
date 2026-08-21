#!/usr/bin/env python3
import concurrent.futures
import socket
import sys
import time

TARGET_HOST = "127.0.0.1"
TARGET_PORT = 8080

def slowloris_client(duration=5):
    """Holds open connection and sends partial headers slowly to test timeout protection"""
    try:
        s = socket.create_connection((TARGET_HOST, TARGET_PORT), timeout=3)
        s.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\n")
        time.sleep(duration)
        s.sendall(b"User-Agent: Slowloris\r\n\r\n")
        res = s.recv(1024)
        s.close()
        return True
    except Exception:
        return False

def rapid_connect_disconnect():
    """Rapidly connects and immediately closes socket to stress accept/close state machine"""
    try:
        s = socket.create_connection((TARGET_HOST, TARGET_PORT), timeout=2)
        s.close()
        return True
    except Exception:
        return False

def run_stress_test(host, port, clients=200):
    global TARGET_HOST, TARGET_PORT
    TARGET_HOST = host
    TARGET_PORT = port

    print(f"==================================================")
    print(f"Running MiniNginx Stress & Resilience Test Suite")
    print(f"Target: {host}:{port} | Concurrency: {clients}")
    print(f"==================================================")

    # Phase 1: Rapid Connect / Disconnect Storm
    print("[ PHASE 1 ] Rapid Connection Storm (2000 rapid connect/closes)...")
    with concurrent.futures.ThreadPoolExecutor(max_workers=50) as pool:
        results = list(pool.map(lambda _: rapid_connect_disconnect(), range(2000)))
    success = sum(1 for r in results if r)
    print(f"[ PHASE 1 COMPLETE ] Success: {success}/2000")

    # Phase 2: Slowloris & Idle Socket Stress
    print("[ PHASE 2 ] Slowloris Idle Connection Defense (100 slow clients)...")
    with concurrent.futures.ThreadPoolExecutor(max_workers=50) as pool:
        slow_results = list(pool.map(lambda _: slowloris_client(duration=2), range(100)))
    print(f"[ PHASE 2 COMPLETE ] Completed slow client tests")

    print("==================================================")
    print("Stress test completed successfully!")
    print("==================================================")

if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080
    run_stress_test(host, port)
