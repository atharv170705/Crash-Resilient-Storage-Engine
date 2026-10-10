# Crash-Resilient Transactional Storage Engine

A C++17 transactional storage engine implementing an append-only Write-Ahead
Log (WAL), binary record serialization, CRC32 validation, transaction
management, and crash recovery.

The project explores how databases preserve committed changes, handle
incomplete transactions, and recover from corrupted or partially written
log records after an unexpected process termination.

## Features

- **Transactional operations:** BEGIN, COMMIT, and ROLLBACK
- **Key-value storage:** SET, GET, and DELETE operations
- **Append-only Write-Ahead Log:** records operations before applying
  committed changes to the in-memory storage engine
- **Binary WAL records:** structured records with magic number, format
  version, transaction ID, operation type, payload, and checksum
- **CRC32 validation:** detects corrupted WAL records during recovery
- **Durable WAL writes:** uses POSIX `write()` and `fsync()` to persist
  log records
- **Crash recovery:** replays operations belonging to committed transactions
- **Incomplete transaction handling:** ignores operations from transactions
  without a valid COMMIT record
- **Invalid-tail repair:** detects incomplete or corrupted trailing WAL
  records and truncates the log to the last valid record
- **Crash-injection testing:** uses `SIGKILL` to test recovery after an
  abrupt process termination
- **Makefile-based build:** compile the project using GNU Make and g++

## Project Architecture

The engine separates storage, transaction handling, WAL management,
binary serialization, record validation, and recovery into individual
components.

```text
                         Database
                            |
             +--------------+--------------+
             |              |              |
        StorageEngine   Transaction     WALManager
             |              |              |
       In-memory KV     Operations     WALSerializer
             |                             |
             |                           CRC32
             |                             |
             +-----------------------------+
                            |
                       WAL on Disk
                            |
                        WALReader
                            |
                     RecoveryManager
                            |
                 Replay Committed Changes
```

### Component Responsibilities

| Component | Responsibility |
|-----------|----------------|
| `Database` | Coordinates storage operations, transactions, WAL writes, and recovery |
| `StorageEngine` | Maintains the in-memory key-value data and provides basic storage operations |
| `Transaction` | Tracks a transaction ID and its operations |
| `WALManager` | Serializes and appends records to the WAL, then synchronizes them using `fsync()` |
| `WALRecord` | Represents a transaction operation stored in the log |
| `WALSerializer` | Converts WAL records to and from a binary representation |
| `CRC32` | Calculates checksums used to validate serialized records |
| `WALReader` | Reads records sequentially and validates their structure and checksum |
| `RecoveryManager` | Identifies committed transactions, replays their operations, and repairs an invalid WAL tail |

## Design and Implementation

### 1. Key-Value Storage

The storage engine maintains key-value pairs using an in-memory
`unordered_map<string, string>`.

The database exposes the following operations:

- `set(key, value)` — records a value assignment inside an active transaction.
- `get(key, value)` — retrieves a value, taking the active transaction's
  pending operations into account.
- `remove(key)` — records a deletion inside an active transaction.

Mutating operations require an active transaction. Changes are applied
to the in-memory storage when the transaction commits.

The engine also contains basic file-based snapshot `save()` and `load()`
methods. The WAL-based recovery mechanism is the primary persistence and
recovery path exercised by the current test harness.

### 2. Transaction Management

Each transaction receives a unique transaction ID. Its operations are
recorded in the WAL and tracked by a `Transaction` object.

The transaction lifecycle is:

```text
BEGIN
  |
  v
SET / DELETE
  |
  +----------------------+
  |                      |
  v                      v
COMMIT                 ROLLBACK
  |                      |
  v                      v
Apply operations       Discard pending
to in-memory storage   transaction state
```

**COMMIT**

1. Appends a COMMIT record to the WAL.
2. Synchronizes the WAL record using `fsync()`.
3. Applies the transaction's recorded SET and DELETE operations to the
   in-memory storage.
4. Clears the active transaction.

**ROLLBACK**

1. Appends a ROLLBACK record to the WAL.
2. Synchronizes the WAL record using `fsync()`.
3. Discards the active transaction's pending operations without applying
   them to the in-memory storage.

The transaction ID and operation history allow the recovery mechanism to
distinguish committed transactions from incomplete or rolled-back ones.

### 3. Write-Ahead Logging (WAL)

The engine uses an append-only log stored at:

```text
data/database.wal
```

Before a transaction's changes are applied to the in-memory storage,
the corresponding operation records are written to the WAL.

The WAL provides the history needed to reconstruct committed changes
after a process restart.

The current implementation uses POSIX file operations:

- `open()` opens or creates the WAL file in append mode.
- `write()` appends a serialized record.
- `fsync()` requests synchronization of the file's data and metadata
  required by the operating system's filesystem implementation.
- `close()` closes the file descriptor.

Each WAL append currently opens the file, writes one record, calls
`fsync()`, and closes the file. Configurable synchronization policies
and group-commit batching are not implemented.

### 4. Binary WAL Record Format

WAL records are serialized into a binary format rather than being stored
as human-readable text.

Each record contains the following fields:

| Field | Size | Purpose |
|-------|------|---------|
| Magic number | 4 bytes | Identifies the WAL record format |
| Format version | 2 bytes | Identifies the serialization version |
| Record length | 4 bytes | Specifies the total serialized record length |
| Transaction ID | 8 bytes | Associates the record with a transaction |
| Operation code | 1 byte | Identifies BEGIN, SET, DELETE, COMMIT, or ROLLBACK |
| Payload | Variable | Contains the key and value for applicable operations |
| CRC32 checksum | 4 bytes | Validates the serialized record |

Multi-byte integers are encoded in little-endian byte order.

The fixed header occupies 10 bytes. The record length allows the reader
to determine how much data belongs to each record, while the magic number
and version help identify malformed or unsupported records.

The serializer validates record lengths, format version, operation codes,
payload bounds, and checksum before accepting a record during recovery.

### 5. CRC32 Validation

Each serialized WAL record contains a CRC32 checksum calculated over
the record bytes excluding the checksum field itself.

When reading a record, the engine recalculates the checksum and compares
it with the stored value.

If the values differ, the record is rejected as corrupted.

CRC32 is used for accidental corruption detection. It is not a
cryptographic integrity mechanism and does not protect against deliberate
tampering.

### 6. Crash Recovery

On startup, `RecoveryManager` scans the WAL and reconstructs the
in-memory state.

The recovery process follows these steps:

1. Read WAL records sequentially.
2. Validate each record's length, format, operation code, and CRC32.
3. Stop at the first incomplete or invalid record.
4. Truncate the invalid trailing portion of the WAL to the last valid
   record boundary.
5. Identify transaction IDs that have a valid COMMIT record in the
   accepted log.
6. Replay SET and DELETE operations belonging to those committed
   transactions.
7. Determine the next transaction ID from the maximum transaction ID
   observed in the accepted records.

Operations belonging to transactions without a COMMIT record are not
replayed. This prevents an incomplete transaction from being applied
during recovery.

The current recovery implementation uses two passes: one to identify
committed transactions and another to replay their operations.

### 7. Invalid WAL Tail Repair

A crash can leave a trailing record incomplete, or a record can fail
checksum validation because its contents are corrupted.

The WAL reader detects conditions such as:

- An incomplete record header
- An incomplete record body
- An invalid record length
- An unsupported format version
- An invalid operation code
- A CRC32 mismatch

The recovery manager truncates the WAL to the byte offset of the last
valid record and synchronizes the truncation.

This allows subsequent database startups and WAL appends to proceed
from the repaired log, rather than repeatedly encountering the same
invalid trailing bytes.

The current implementation repairs an invalid tail in a single WAL
file. Multi-file WAL segmentation is not implemented.

## Crash and Corruption Tests

The project includes test modes in `main.cpp` for checking recovery
behavior across separate process executions.

Build the executable first:

```bash
make
```

### 1. Crash Recovery Test

Run:

```bash
./minidb crash-test
```

The test:

1. Removes the existing WAL file.
2. Commits `A = 100`.
3. Starts another transaction and writes `B = 200` without committing.
4. Terminates the process using `SIGKILL`.

The process is expected to terminate abruptly before normal cleanup.

Restart the executable in verification mode:

```bash
./minidb verify-crash
```

Expected result:

```text
A: 100
B: missing

Crash recovery test: PASSED
```

This checks that the committed transaction is recovered while the
uncommitted transaction is not replayed.

### 2. Invalid WAL Tail Repair Test

Run:

```bash
./minidb tail-test
```

The test commits `A = 100`, appends invalid bytes to the WAL, and
restarts the database to trigger tail validation and repair. It then
commits `B = 200`.

Verify the result after another restart:

```bash
./minidb verify-tail
```

Expected result:

```text
A: 100
B: 200

WAL tail repair test: PASSED
```

This checks that a valid committed transaction survives tail repair
and that new transactions can be appended afterward.

### 3. CRC32 Corruption Test

Run:

```bash
./minidb crc-test
```

The test creates two committed transactions:

- `A = 100`
- `B = 200`

It then corrupts the final byte of the WAL, which belongs to the CRC32
checksum of the last COMMIT record.

On restart, recovery should reject and truncate the corrupted record.
Because the second transaction's COMMIT record is no longer valid,
its operations should not be replayed.

Verify the result:

```bash
./minidb verify-crc
```

Expected result:

```text
A: 100
B: missing

CRC corruption test: PASSED
```

These are manually invoked test scenarios, not a large-scale automated
crash-injection or benchmark suite.

## Build and Run

### Prerequisites

- A C++17-compatible compiler, such as `g++`
- GNU Make
- A POSIX-compatible environment providing `open()`, `write()`,
  `fsync()`, `ftruncate()`, and process signals

### Build

From the project root:

```bash
make
```

This compiles the source files into the `minidb` executable.

### View Available Test Modes

```bash
./minidb
```

The executable prints its supported test modes when invoked without
a mode argument.

### Run a Test

For example:

```bash
./minidb crash-test
```

Then run its corresponding verification mode:

```bash
./minidb verify-crash
```

The same pattern applies to the tail-repair and CRC32 corruption tests.

**Note:** the test modes modify `data/database.wal`. Run them in a
development environment where existing WAL data can safely be replaced.

### Clean

Remove the compiled executable:

```bash
make clean
```

## Repository Structure

```text
Storage Engine/
├── include/
│   ├── crc32.hpp
│   ├── database.hpp
│   ├── recovery_manager.hpp
│   ├── storage_engine.hpp
│   ├── transaction.hpp
│   ├── wal_manager.hpp
│   ├── wal_reader.hpp
│   ├── wal_record.hpp
│   └── wal_serializer.hpp
│
├── src/
│   ├── crc32.cpp
│   ├── database.cpp
│   ├── recovery_manager.cpp
│   ├── storage_engine.cpp
│   ├── transaction.cpp
│   ├── wal_manager.cpp
│   ├── wal_reader.cpp
│   └── wal_serializer.cpp
│
├── data/
│   └── database.wal          # Generated at runtime
│
├── main.cpp
├── Makefile
├── .gitignore
└── README.md
```

## Technologies

- **C++17**
- **GNU Make and g++**
- **STL containers:** `unordered_map`, `vector`, and `unordered_set`
- **Binary serialization:** `uint8_t`, fixed-width integer types, and byte buffers
- **CRC32:** record integrity validation
- **POSIX file APIs:** `open()`, `write()`, `fsync()`, `ftruncate()`, and `close()`
- **Process signals:** `SIGKILL` for crash testing
- **RAII and OOP:** classes with separated responsibilities and
  `unique_ptr`-managed transaction state

## Current Scope and Limitations

This project focuses on the fundamentals of transactional storage,
WAL persistence, binary record validation, and restart recovery.

The current implementation does not include:

- Configurable `fsync()` batching or group commit
- WAL segmentation or multi-segment recovery
- A measured throughput benchmark suite
- Concurrent transaction execution or multi-process coordination
- A disk-based page manager or B-tree index
- A full SQL parser or query engine
- A comprehensive automated test framework

The recovery logic is designed for the current single-file WAL format.
It should not be interpreted as a complete production database durability
implementation. In particular, the tests exercise process termination
and deliberate file corruption; they do not establish guarantees against
every possible hardware, filesystem, or power failure.

## Key Learnings

Working on this project provided practical experience with:

- Modeling transaction state and commit/rollback behavior
- Understanding the Write-Ahead Logging approach to persistence
- Designing and validating a binary record format
- Using checksums to detect corrupted log records
- Using POSIX system calls for file writes and synchronization
- Reconstructing committed state from an operation log
- Handling incomplete and corrupted WAL tails during recovery
- Testing crash-related behavior by terminating and restarting processes

## Future Improvements

Possible extensions include:

- Configurable synchronization policies and group commit
- WAL segmentation with recovery across multiple log files
- Automated tests for transaction semantics, malformed records, and
  additional crash points
- Benchmarks for throughput, latency, and recovery time
- More robust handling of filesystem errors and partial writes
- A snapshot/checkpoint mechanism to reduce recovery work
- Stronger durability guarantees around directory metadata and file creation
- More extensive validation of transaction state transitions
