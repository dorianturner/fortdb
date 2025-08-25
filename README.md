FortDB — Fully Versioned, Crash‑Proof, Hierarchical NoSQL Database
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

* **Append-only writes**: SET/DELETE always append; no in-place updates, ensuring crash safety.
* **Hierarchical versioning**: VersionNode chains at every level.
* **Local versions**: `uint64_t` counters track per-entity changes.
* **Time-travel reads**: Query any historical state with `--v` flag.
* **Atomic compaction**: Background process compacts data and swaps files atomically.
* **Thread-safe**: RW-locks on structures and global mutex for version counter.

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

