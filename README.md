# IN ACTIVE DEVELOPMENT : NOT FINISHED

FortDB — Fully Versioned, Crash‑Proof, Hierarchical NoSQL Database

** This is a WORK IN PROGRESS and is currently buggy etc. etc.**

**Interactive Shell Usage**

1. **Start FortDB**

   ```bash
   $ ./fortdb
   fortdb started. Type 'exit' or 'quit' to quit.
   fortdb>
   ```

2. **Supported Commands**

   | Command                     | Example                        | Description                                     |
   | --------------------------- | ------------------------------ | ----------------------------------------------- |
   | `load <path>`               | `load /home/me/db.fort`        | Load database from file                         |
   | `get <path> [--v=<V>]`      | `get users/john/age`           | Fetch field value (optional global version `V`) |
   | `set <path> <value>`        | `set users/john/age 42`        | Insert or update field                          |
   | `delete <path>`             | `delete users/john/age`        | Tombstone an entity                             |
   | `list-versions <path>`      | `list-versions users/john/age` | List all versions of an entity                  |
   | `compact <path>`            | `compact users/john`           | Retain only latest versions, remove tombstones  |
   | `save <path>`               | `save /home/me/db.fort`        | Save current in-memory DB to file               |
   | `exit` or `quit`            | `exit`                         | Exit the interactive shell                      |

3. **Key Features**

* **Append-only writes**: SET/DELETE always append; no in-place updates, ensuring crash safety.
* **Hierarchical versioning**: VersionNode chains at Table, Collection, Document, and Field levels.
* **Global & local versions**: `uint64_t` counters track DB-wide and per-entity changes.
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
string:alice@example.com
fortdb> set users/alice/age 30
OK
fortdb> list-versions users/alice/age
v1: 25
v2: 30
fortdb> compact users/alice
Compacted users/alice
fortdb> save db.fort
Saved database to db.fort
fortdb> exit
```

5. **Getting Help**

Type `help` or `?` at the prompt for command summaries.

