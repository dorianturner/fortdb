# FortDB — Simple, Crash‑Proof, Versioned Key‑Value Database

## 1. Problem We Solve

Traditional databases perform in‑place updates. If the system crashes mid‑write, data files can become corrupt, requiring complex recovery (WALs, undo logs, etc.).

**FortDB’s solution**: never overwrite. All operations append to a log, making writes atomic and crash‑safe, with built‑in version history.

---

## 2. Core Features

1. **Append‑only writes**: All `SET` and `DELETE` operations append records to disk; no in‑place file modifications.
2. **Global versioning**: A DB‑wide counter increments on every write, tagging each record with a unique global version.
3. **Local (per‑cell) versioning**: Each key maintains its own version counter, incremented on each update, for quick local history.
4. **Tombstone deletions**: `DELETE` writes a tombstone record; cleanup occurs later via compaction.
5. **Time‑travel reads**: Queries can request state at any global version `V`.
6. **Atomic compaction**: Background process rewrites live records to a new file, then atomically swaps it in.
7. **Thread‑safe multi‑user access**: Table‑level locks ensure safe concurrent reads/writes.

---

## 3. Versioning Model

* **Global version** (`uint64_t`): increments on each `SET` or `DELETE`. Used for snapshot isolation and compaction boundaries.
* **Local version** (`uint64_t`): increments only for that specific key on each write. Used to track per‑cell changes efficiently.

Each record stored on disk includes both versions.

---

## 4. Supported CLI Commands

```
./fortdb set <key> <value>         # Appends SET record (increments global&local)
./fortdb get <key> [--v=<V>]       # Reads latest or at version V
./fortdb delete <key>              # Appends tombstone (increments global&local)
./fortdb list-versions <key>       # Lists all (global,local,value)
./fortdb compact                   # Runs compaction on all tables
```

*Example:* `./fortdb set users:1:name "Alice"`

---

## 5. Internal Data Layout


### 5.1 In‑Memory Index

A nested hashmap structure for O(1) lookups:

* **tables** maps table names (prefixes) to tables.
* **rows** maps row IDs (e.g. `"1"`) to row instances.
* **cols** maps column names (e.g. `"name"`) to cells.
* **head** points to the latest VersionNode; older versions link via `prev`.

---

## 6. Read & Write Paths

### SET/DELETE

1. Acquire table write lock.
2. Increment global version and cell.local\_counter.
3. Append `Record` to disk, flush.
4. In memory: create new `VersionNode`, link to cell.head, update head.
5. Release lock.

### GET (optional `--v=<V>`)

1. Acquire table read lock.
2. Lookup `Cell` at tables→rows→cols.
3. Starting at head, traverse `VersionNode` until `global_version ≤ V` (or return head for latest).
4. If node is tombstone or none found, return NOT\_FOUND; else return value.
5. Release lock.

---

## 7. Compaction

1. Pause writes (acquire global compaction lock).
2. Create new log file.
3. For each table, row, col: copy only latest non‑tombstone head into new file (with versions).
4. Flush and fsync new file.
5. Atomically swap file paths.
6. Reload indexes from the compacted file.
7. Resume writes.

---

## 8. Thread Safety & Concurrency

* **Table‑level rwlocks** allow multiple readers or one writer.
* **Global version counter** protected by a mutex.
* Writes are serialized per table; reads proceed concurrently.

---

## 9. Future Extensions

* SQL translation layer (later).
* Lock‑free data structures for higher concurrency.
* Distributed replication and sharding.
