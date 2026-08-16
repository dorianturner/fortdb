FortDB — Fully Versioned, Thread-Safe, Hierarchical NoSQL Database
**Interactive Shell Usage**

1. **Start FortDB**

   ```bash
   $ ./fortdb
   fortdb started. Type 'exit' or 'quit' to quit.
   fortdb>
   ```

2. **Supported Commands**

   | Command                | Example                        | Description                                    |
   | ---------------------- | ------------------------------ | ---------------------------------------------- |
   | `load <path>`          | `load /home/me/db.fort`        | Load database from file                        |
   | `get <path> [--v=<V>]` | `get users/john/age`           | Fetch field value (optional local version `V`) |
   | `set <path> <value>`   | `set users/john/age 42`        | Insert or update field                         |
   | `delete <path>`        | `delete users/john/age`        | Tombstone an entity                            |
   | `list-versions <path>` | `list-versions users/john/age` | List all versions of an entity                 |
   | `compact <path>`       | `compact users/john`           | Retain only latest versions, remove tombstones |
   | `compact_db`           | `compact_db`                   | Compact entire database                        |
   | `save <path>`          | `save ./test/saves/db.fort`    | Save current in-memory DB to file              |
   | `exit`, `quit`         | `exit`                         | Exit the interactive shell                     |
   | `dump`                 | `dump`                         | Print the entire database state to the console |
   | `help`, `?`            | `help`                         | Show this help message                         |

3. **Key Features**

* **Append-only writes**: SET/DELETE always append; no in-place updates.
* **Hierarchical versioning**: VersionNode chains at every level.
* **Local versions**: `uint64_t` counters track per-entity changes.
* **Time-travel reads**: Query any historical state with `--v` flag.
* **Atomic persistence**: `save` serializes a locked snapshot to a same-directory temporary file, flushes it, and renames it into place.
* **Thread-safe**: Document reads, writes, serialization, compaction, and document lifetime pins are synchronized with read/write locks and reference counts.
* **Immutable history**: Existing payloads and version nodes are never edited by writes; only compaction detaches and releases older chains.

4. **Example Session**

```bash
$ ./fortdb
fortdb started. Type 'exit' or 'quit' to quit.
fortdb> load db.fort
Loaded database from db.fort
fortdb> get users/alice/email
alice@example.com
fortdb> get users/alice 
fields: name, age
subdocuments: children
fortdb> set users/alice/age 30
OK
fortdb> list-versions users/alice/age
v3: 30
v2: <deleted>
v1: 29
fortdb> compact users/alice
Compacted users/alice
fortdb> save db.fort
Saved database to db.fort
fortdb> exit
```

5. **Getting Help**

Type `help` at the prompt for command summaries.

6. **Future Plans**

* Provide a live server CLI for concurrent reads and writes from multiple users
