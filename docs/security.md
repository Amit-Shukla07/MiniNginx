# Security Architecture & Defensive Mechanisms

## 1. Directory Traversal Defense

Path traversal attacks (such as `GET /../../../../etc/passwd` or `GET /assets/../../shadow`) are a critical vulnerability in custom web servers.

### MiniNginx Protection Mechanism:
1. Resolves canonical root using `std::filesystem::weakly_canonical(doc_root)`.
2. Resolves canonical target using `std::filesystem::weakly_canonical(target_path)`.
3. Verifies that the normalized string prefix of the target **strictly begins with** the canonical root string:
```cpp
std::string root_str = canonical_root_.lexically_normal().string();
std::string target_str = canonical_target.lexically_normal().string();

if (target_str.rfind(root_str, 0) != 0) {
    // Attack detected! Reject immediately
    return std::nullopt;
}
```
4. Verifies that the file exists and is a regular file (`is_regular_file`), rejecting symbolic links or directories escaping the document root.

---

## 2. Slowloris & Idle Socket Exhaustion Protection

* **Vulnerability**: An attacker opens thousands of connections and sends partial headers slowly (e.g. 1 byte every 5 seconds), exhausting server file descriptor tables.
* **MiniNginx Defense**:
  - `TimerManager` sets an inactivity timer (default 10s for requests, 60s for keep-alive).
  - Every connection tracks activity timestamps.
  - If a client fails to transmit a full HTTP request within the deadline, the min-heap timer fires and automatically closes the socket, reclaiming kernel resources.

---

## 3. Resource Bounds & Denial-of-Service Mitigations

* **Header Size Limit**: Max 64 KB (rejects with `431 Request Header Fields Too Large`).
* **URI Length Limit**: Max 8,192 bytes (rejects with `414 URI Too Long`).
* **Header Count Limit**: Max 128 headers per request.
* **Body Size Limit**: Max 10 MB (rejects with `413 Payload Too Large`).
* **Max Connections Limit**: Rejects new incoming TCP connections beyond `max_connections` (default 10,000) to protect process limits.
* **SIGPIPE Suppression**: Explicitly sets `std::signal(SIGPIPE, SIG_IGN)` to prevent process crashes when writing to broken client sockets.
