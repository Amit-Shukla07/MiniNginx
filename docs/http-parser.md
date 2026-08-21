# Incremental HTTP/1.1 Parser Design

## 1. Design Philosophy

Standard HTTP parsers in tutorials often read the entire request into a string using a single `recv()` call. In real TCP networks, this is fundamentally broken because:
1. **TCP is a stream protocol, not a message protocol**: IP packets can fragment at any byte boundary (e.g. `GET /inde` in packet 1, `x.html HTTP/1.1\r\n` in packet 2).
2. **Pipelining**: Multiple requests can arrive in a single `read()` syscall.
3. **Slowloris attacks**: Malicious clients can send headers 1 byte every 5 seconds.

MiniNginx solves this with a **Deterministic Finite-State Machine (FSM)** working directly on incremental dynamic buffers.

---

## 2. State Machine Transitions

```
 [Start]
    │
    ▼
 REQUEST_LINE ──── (Incomplete) ────► [Wait for more bytes in Buffer]
    │
    ├─ (Valid CRLF)
    ▼
  HEADERS     ──── (Incomplete) ────► [Wait for more bytes in Buffer]
    │
    ├─ (Blank line \r\n\r\n)
    │
    ├──────────┬───────────────────┬──────────────────────┐
    ▼          ▼                   ▼                      ▼
  No Body    Content-Length > 0  Transfer-Encoding: chunked  Malformed
    │          │                   │                      │
    ▼          ▼                   ▼                      ▼
 COMPLETE     BODY            CHUNK_HEADER              ERROR
               │                   │                      │
         (Read N bytes)      (Parse hex size)      (400/413/414/431)
               │                   │                      │
               ▼                   ▼                      ▼
            COMPLETE            COMPLETE             Send Error & Close
```

---

## 3. Strict RFC & Defensive Bounds

| Condition | RFC Specification | MiniNginx Enforced Limit | HTTP Error Code |
| :--- | :--- | :--- | :--- |
| URI Length | RFC 9112 Sec 3 | 8,192 bytes | `414 URI Too Long` |
| Total Headers Size | RFC 9112 Sec 2.1 | 65,536 bytes | `431 Request Header Fields Too Large` |
| Header Count | RFC 9110 | 128 headers | `431 Request Header Fields Too Large` |
| Body Size | Configurable | 10 MB (default) | `413 Payload Too Large` |
| HTTP Version | RFC 9112 | Must be HTTP/1.0 or HTTP/1.1 | `505 HTTP Version Not Supported` |
| Method | RFC 9110 | GET, HEAD, POST, PUT, DELETE | `501 Not Implemented` |
