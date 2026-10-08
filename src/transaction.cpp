#include "transaction.hpp"

Transaction::Transaction(uint64_t txn_id) 
    : txn_id(txn_id) {

}

uint64_t Transaction::getId() const {
    return txn_id;
}

void Transaction::addOperation(const WALRecord& record) {
    operations.push_back(record);
}

const vector<WALRecord>& Transaction::getOperations() const {
    return operations;
}