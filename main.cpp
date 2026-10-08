#include <bits/stdc++.h>
#include "database.hpp"

#include "wal_manager.hpp"
#include "wal_serializer.hpp"
#include "wal_reader.hpp"
#include "recovery_manager.hpp"

using namespace std;

int main() {
    string walFile = "data/recovery_test.wal";

    // Create a clean WAL for this test.
    remove(walFile.c_str());

    WALManager wal(walFile);

    // TXN 1: committed.
    wal.append(WALRecord(1, OpCode::BEGIN));
    wal.append(WALRecord(1, OpCode::SET, "A", "100"));
    wal.append(WALRecord(1, OpCode::SET, "B", "200"));
    wal.append(WALRecord(1, OpCode::COMMIT));

    // TXN 2: incomplete.
    wal.append(WALRecord(2, OpCode::BEGIN));
    wal.append(WALRecord(2, OpCode::SET, "C", "300"));

    // Simulate crash here.
    // No COMMIT for TXN 2.

    StorageEngine storage;

    RecoveryManager recovery(walFile);

    uint64_t maxTxnId = recovery.recover(storage);

    cout << "Highest transaction ID: " << maxTxnId << "\n\n";

    string value;

    if (storage.get("A", value)) {
        cout << "A = " << value << "\n";
    }

    if (storage.get("B", value)) {
        cout << "B = " << value << "\n";
    }

    if (storage.get("C", value)) {
        cout << "C = " << value << "\n";
    }
    else {
        cout << "C does not exist\n";
    }

    return 0;
}