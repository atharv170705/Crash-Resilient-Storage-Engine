#include "recovery_manager.hpp"

using namespace std;

RecoveryManager::RecoveryManager(const string& walFilename)
    : reader(walFilename) {
}

uint64_t RecoveryManager::recover(StorageEngine& storage) {
    vector<WALRecord> records = reader.readAll();

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