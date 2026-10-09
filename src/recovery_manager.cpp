#include "recovery_manager.hpp"
#include <fcntl.h>
#include <unistd.h>

using namespace std;

RecoveryManager::RecoveryManager(const string& walFilename)
    : filename(walFilename),
      reader(walFilename) {
}

void RecoveryManager::truncateWAL(uint64_t validBytes) {
    int fd = open(filename.c_str(), O_WRONLY);

    if (fd == -1) {
        throw runtime_error("Failed to open WAL for truncation");
    }

    if (ftruncate(fd, static_cast<off_t>(validBytes)) == -1) {
        close(fd);
        throw runtime_error("Failed to truncate invalid WAL tail");
    }

    if (fsync(fd) == -1) {
        close(fd);
        throw runtime_error("Failed to fsync truncated WAL");
    }

    close(fd);
}

uint64_t RecoveryManager::recover(StorageEngine& storage) {
    uint64_t validBytes = 0;
    bool invalidTail = false;

    vector<WALRecord> records = reader.readAll(validBytes, invalidTail);

    if (invalidTail) {
        truncateWAL(validBytes);
        cout << "Truncated invalid WAL tail at byte " << validBytes << "\n";
    }

    unordered_set<uint64_t> committedTransactions;
    uint64_t maxTxnId = 0;

    // First pass: identify committed transactions.
    for(const WALRecord& record : records) {
        maxTxnId = max(maxTxnId, record.txn_id);
        if (record.op == OpCode::COMMIT) {
            committedTransactions.insert(record.txn_id);
        }
    }

    // Second pass: replay operations belonging to committed transactions.
    for (const WALRecord& record : records) {
        if (committedTransactions.find(record.txn_id) == committedTransactions.end()) {
            continue;
        }

        if (record.op == OpCode::SET) {
            storage.set(record.key, record.value);
        }
        else if (record.op == OpCode::DELETE_KEY) {
            storage.remove(record.key);
        }
    }

    return maxTxnId;
}