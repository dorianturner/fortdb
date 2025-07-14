## Utils Module Design

### 1. Purpose

Provide append‑only, versioned key–value storage inspired by Firestore’s NoSQL model. Ensures immutability by never altering existing values in place.

### 2. Data Model

* **Key**: Unique string identifier representing a document path (e.g., `users/123/profile`).
* **Entry**: Hash bucket node linking a key to its latest `VersionNode`.
* **VersionNode**: Immutable snapshot of a value at a specific version:

  * `payload`: Actual data blob (e.g., JSON document)
  * `version`: Monotonic counter indicating update order
  * `prev`: Pointer to previous `VersionNode`

### 3. General Functions

* **hashmap\_create(bucket\_count)**

  * Allocates and initializes the bucket array and counters.
* **hashmap\_put(map, key, new\_payload, global\_version, free\_value)**

  1. Compute bucket index via hash of `key`.
  2. Find or create an `Entry` for `key`.
  3. Allocate a new `VersionNode`:

     * Copy `new_payload`
     * Set `version = global_version`
     * Link `prev` to the entry’s existing node
  4. Update `entry->value` to new node.
  5. Optionally call `free_value` on older data if pruning.
  6. Increment `global_version` externally.
* **hashmap\_get(map, key, local\_version)**

  1. Locate `Entry` by `key`.
  2. Traverse its `VersionNode` chain until finding the highest `version <= local_version`.
  3. Return the corresponding `payload` or `NULL` if not found.
* **hashmap\_free(map, free\_value)**

  * Iterate all buckets and entries, freeing each `VersionNode` chain via `free_value`, then deallocate structures.

### 4. VersionNode Usage

Main Point: Anytime we would modify memory inplace, we instead add a new version node with the new value and link it back.
1. **Immutability**: Existing nodes are read‑only, preventing accidental in‑place changes.
2. **Snapshot Reads**: Clients can specify a `local_version` to view historical states.
3. **Chaining**: Each update spawns a new node linked to its predecessor, forming a time‑ordered chain.
4. **Pruning**: Safe removal of older nodes when no longer needed by any reader.

### 5. NoSQL Document–Collection Example

Imagine a Firestore‑style hierarchy nested in keys:

```
users/{userId}/
  profile: { name, email, ... }
  settings: { theme, notifications }
users/{userId}/orders/{orderId}/
  orderFields: { items, total, status }
```

* **Documents** ↔ `payload` JSON blobs in `VersionNode`s.
* **Collections** ↔ Sets of keys sharing a path prefix (e.g., `users/123/orders`).
* **Snapshots** ↔ Selecting all entries with `version <= v` yields a consistent view across documents.

A root table stores collections
Collections store Documents
Documents store fields, and sub collections

### 6. Caveats

* **Memory Growth**: Chain length grows with each update—plan pruning strategy. Requires compaction to keep store size down.
* **Thread Safety**: Requires external synchronization for concurrent access.
* **Version Overflow**: Monitor counter wrap‑around in long‑lived systems.
