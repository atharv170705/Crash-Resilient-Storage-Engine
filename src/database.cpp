#include "database.hpp"

using namespace std;

Database::Database()
    : wal("data/database.wal"),
      nextTxnId(1) {

}

void Database::begin() {
    if (activeTransaction != nullptr) {
        throw runtime_error("Transaction already active");
    }
    activeTransaction = make_unique<Transaction>(nextTxnId);
    WALRecord record(nextTxnId, OpCode::BEGIN);
    wal.append(record);
    nextTxnId++;
}

void Database::set(const string& key, const string& value) {
    if (activeTransaction == nullptr) {
        throw runtime_error("No active transaction");
    }
    WALRecord record(activeTransaction -> getId(), OpCode::SET, key, value);
    wal.append(record);
    activeTransaction -> addOperation(record);
}

bool Database::remove(const string& key) {
    if (activeTransaction == nullptr) {
        throw runtime_error("No active transaction");
    }

    const auto& operations = activeTransaction -> getOperations();

    for (auto it = operations.rbegin(); it != operations.rend(); it++) {
        if (it->key != key) {
            continue;
        }

        if (it->op == OpCode::SET) {
            WALRecord record(activeTransaction->getId(), OpCode::DELETE_KEY, key);
            wal.append(record);
            activeTransaction->addOperation(record);
            return true;
        }

        if (it->op == OpCode::DELETE_KEY) {
            return false;
        }
    }

    string existingValue;

    if (!storage.get(key, existingValue)) {
        return false;
    }

    WALRecord record(activeTransaction->getId(), OpCode::DELETE_KEY, key);
    wal.append(record);
    activeTransaction->addOperation(record);

    return true;
}


void Database::commit() {
    if (activeTransaction == nullptr) {
        throw runtime_error("No active transaction");
    }

    uint64_t txnId = activeTransaction->getId();
    WALRecord commitRecord(txnId, OpCode::COMMIT);
    wal.append(commitRecord);

    for (const WALRecord& record : activeTransaction->getOperations()) {
        if (record.op == OpCode::SET) {
            storage.set(record.key, record.value);
        }
        else if (record.op == OpCode::DELETE_KEY) {
            storage.remove(record.key);
        }
    }

    activeTransaction.reset();
}

void Database::rollback() {
    if (activeTransaction == nullptr) {
        throw runtime_error("No active transaction");
    }

    uint64_t txnId = activeTransaction->getId();
    WALRecord rollbackRecord(txnId, OpCode::ROLLBACK);
    wal.append(rollbackRecord);
    activeTransaction.reset();
}

bool Database::get(const string& key, string& value) const {
    if (activeTransaction != nullptr) {
        const auto& operations = activeTransaction -> getOperations();

        for (auto it = operations.rbegin(); it != operations.rend(); it++) {
            if (it->key != key) {
                continue;
            }
            if (it->op == OpCode::SET) {
                value = it->value;
                return true;
            }
            if (it->op == OpCode::DELETE_KEY) {
                return false;
            }
        }
    }

    return storage.get(key, value);
}

void Database::save() const {
    storage.save();
}

void Database::load() {
    storage.load();
}