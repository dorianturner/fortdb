# fortdb
FortDB - The Crash-Proof Database

Problem: Modern databases rely on complex WAL (Write-Ahead Logging) for crash recovery because they perform risky in-place modifications.

Solution: FortDB sidesteps corruption risks entirely with an append-only, versioned design. Deletions are handled safely via tombstoning and atomic compaction.

→ No recovery logic needed (WAL/undo logs).
→ Time-travel queries for historical data.

Key Features
Append-only writes: No in-place updates → crash-proof.

Tombstone deletion: DELETE appends a marker; no physical deletes.

Atomic compaction: Removes dead entries without blocking reads.

Zero recovery logic: No WAL or undo logs.

Time-travel queries: Optional versioned reads (V=1).
