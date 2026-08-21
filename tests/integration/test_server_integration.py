#!/usr/bin/env python3
import http.client
import json
import os
import signal
import socket
import subprocess
import sys
import time

SERVER_BIN = "./build/mininginx"
SERVER_PORT = 8989
DOC_ROOT = "./public"

def wait_for_server(port, timeout=5.0):
    start = time.time()
    while time.time() - start < timeout:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return True
        except (socket.error, ConnectionRefusedError):
            time.sleep(0.05)
    return False

def run_all_tests():
    print("==================================================")
    print("Starting MiniNginx Live Integration Test Suite")
    print("==================================================")

    if not os.path.exists(SERVER_BIN):
        print(f"Error: Server binary '{SERVER_BIN}' not found. Run 'make build' first.")
        sys.exit(1)

    # Spawn MiniNginx server
    proc = subprocess.Popen(
        [SERVER_BIN, "-p", str(SERVER_PORT), "-r", DOC_ROOT, "-w", "2"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    try:
        if not wait_for_server(SERVER_PORT):
            print("Failed to connect to MiniNginx server!")
            proc.kill()
            sys.exit(1)

        print("[ PASS ] Server started and listening on port", SERVER_PORT)

        # Test 1: GET /
        conn = http.client.HTTPConnection("127.0.0.1", SERVER_PORT)
        conn.request("GET", "/")
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200, got {res.status}"
        body = res.read().decode("utf-8")
        assert "MiniNginx" in body, "Body missing MiniNginx brand"
        print("[ PASS ] Test 1: GET / (index.html)")

        # Test 2: GET /style.css
        conn.request("GET", "/style.css")
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200, got {res.status}"
        assert "text/css" in res.getheader("Content-Type", ""), "MIME type mismatch"
        res.read()
        print("[ PASS ] Test 2: GET /style.css (MIME detection)")

        # Test 3: GET /api/health
        conn.request("GET", "/api/health")
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200, got {res.status}"
        data = json.loads(res.read().decode("utf-8"))
        assert data.get("status") == "ok", "Health status invalid"
        print("[ PASS ] Test 3: GET /api/health (JSON API)")

        # Test 4: GET /api/stats
        conn.request("GET", "/api/stats")
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200, got {res.status}"
        stats = json.loads(res.read().decode("utf-8"))
        assert "total_requests" in stats and stats["total_requests"] >= 3
        print("[ PASS ] Test 4: GET /api/stats (Runtime metrics)")

        # Test 5: HEAD /
        conn.request("HEAD", "/")
        res = conn.getresponse()
        assert res.status == 200, f"Expected 200, got {res.status}"
        assert res.getheader("Content-Length") is not None
        head_body = res.read()
        assert len(head_body) == 0, "HEAD response must not contain a body"
        print("[ PASS ] Test 5: HEAD / (RFC compliance)")

        # Test 6: GET /missing
        conn.request("GET", "/non_existent_file_404.html")
        res = conn.getresponse()
        assert res.status == 404, f"Expected 404, got {res.status}"
        res.read()
        print("[ PASS ] Test 6: GET 404 handler")

        # Test 7: Path traversal exploit defense
        conn.request("GET", "/../../../../../../etc/passwd")
        res = conn.getresponse()
        assert res.status == 404 or res.status == 403, f"Path traversal should fail, got {res.status}"
        res.read()
        print("[ PASS ] Test 7: Path Traversal Attack Defense (/../../etc/passwd)")

        conn.close()

        # Test 8: Persistent Keep-Alive (5 requests on 1 raw socket)
        s = socket.create_connection(("127.0.0.1", SERVER_PORT))
        for i in range(5):
            req = f"GET /api/health HTTP/1.1\r\nHost: 127.0.0.1:{SERVER_PORT}\r\nConnection: keep-alive\r\n\r\n"
            s.sendall(req.encode("utf-8"))
            data = s.recv(4096).decode("utf-8")
            assert "200 OK" in data, f"Keep-alive request {i} failed"
            assert "keep-alive" in data.lower()
        s.close()
        print("[ PASS ] Test 8: Persistent Keep-Alive Connection (5 sequential requests)")

        # Test 9: Pipelining (2 concatenated requests in 1 send)
        s = socket.create_connection(("127.0.0.1", SERVER_PORT))
        pipe_req = (
            f"GET /api/health HTTP/1.1\r\nHost: localhost\r\n\r\n"
            f"GET /api/stats HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
        )
        s.sendall(pipe_req.encode("utf-8"))
        received = b""
        while True:
            chunk = s.recv(4096)
            if not chunk: break
            received += chunk
        s.close()
        rec_str = received.decode("utf-8", errors="ignore")
        assert rec_str.count("200 OK") == 2, "Pipelined requests did not both return 200 OK"
        print("[ PASS ] Test 9: HTTP Pipelining (Multiple requests in single packet)")

        # Test 10: Graceful Shutdown
        proc.send_signal(signal.SIGINT)
        proc.wait(timeout=3)
        assert proc.returncode == 0 or proc.returncode == -2, f"Expected clean shutdown code, got {proc.returncode}"
        print("[ PASS ] Test 10: Graceful Shutdown (SIGINT handling)")

        print("==================================================")
        print("ALL 10 INTEGRATION TESTS PASSED SUCCESSFULLY! (100%)")
        print("==================================================")

    finally:
        if proc.poll() is None:
            proc.kill()

if __name__ == "__main__":
    run_all_tests()
