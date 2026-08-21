# Connection Lifecycle State Machine

## 1. State Diagram

```
       [TCP Accept]
            │
            ▼
        CONNECTED
            │
            ▼
         READING ◄──────────────────────────────┐
            │                                   │ (Keep-Alive reset)
            ▼                                   │
         PARSING ── (Incomplete) ──► [Wait]     │
            │                                   │
       (Complete)                               │
            │                                   │
            ▼                                   │
         ROUTING                                │
            │                                   │
            ▼                                   │
         WRITING                                │
            │                                   │
    (write_buf empty)                           │
            │                                   │
     (Keep-Alive?)                              │
      ├── Yes ──────────────────────────────────┘
      └── No
            │
            ▼
         CLOSING
            │
            ▼
         CLOSED (FD closed, timers cancelled, removed from loop)
```

---

## 2. Resource Management & RAII

* Every connection is managed via `std::shared_ptr<Connection>` with `enable_shared_from_this`.
* Event callbacks capture `weak_ptr` or `shared_ptr` to ensure lifetime safety during asynchronous dispatch.
* When a connection closes:
  1. It transitions to `ConnectionState::CLOSED` (idempotent).
  2. Cancels its active timer in `TimerManager`.
  3. Deregisters its file descriptor from the `EventLoop`.
  4. Explicitly closes `Socket`.
  5. Atomically decrements the `active_connections` metric.
  6. Removes itself from the `Worker` connection table.
