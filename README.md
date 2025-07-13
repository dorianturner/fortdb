````markdown
FortDB — Fully Versioned, Crash‑Proof, Hierarchical NoSQL Database

1. Problem We Solve

   Traditional DBs overwrite in place. Crashes mid‑write corrupt data, requiring complex recovery (WALs, undo logs, etc.).

   FortDB’s solution: never overwrite—append every change to a log. All entities (tables, collections, documents, fields) are versioned, making writes atomic, crash‑safe, and time‑travelable.

2. Core Features

   - **Append‑only writes**: Every SET/DELETE appends a record; no in‑place updates.
   - **Full hierarchical versioning**:  
     • Tables → VersionNode chain  
     • Collections → VersionNode chain  
     • Documents → VersionNode chain  
     • Fields → VersionNode chain  
   - **Global versioning**: DB‑wide counter increments on each write.
   - **Local versioning**: Each entity’s own counter increments per update.
   - **Tombstone deletions**: DELETE appends a tombstone; cleaned up in compaction.
   - **Time‑travel reads**: Query any global version V on any entity.
   - **Atomic compaction**: Background compaction writes live versions to new file, then atomically swaps.
   - **Thread‑safe multi‑user**: RW‑locks at Table and Collection levels; global mutex for version counter.

3. Versioning Model

   - **Global version** (`uint64_t`): increments per SET/DELETE, tags every record.
   - **Local version** (`uint64_t`): per‑entity counter for quick local history.
   - **VersionNode**: generic struct `{ value, global_version, local_version, prev }` used for tables, collections, documents, fields.

4. Supported CLI Commands

   ```bash
   ./fortdb set <path> <value>        # path: table/collection/.../field
   ./fortdb get <path> [--v=<V>]      # read a field at version V
   ./fortdb delete <path>             # tombstone an entity
   ./fortdb list-versions <path>      # list all versions of any entity
   ./fortdb compact                   # compact entire hierarchy to latest db version
````

Example: `./fortdb set users/123/posts/456/title "Hello"`

```
```

