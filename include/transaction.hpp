#pragma once

#include <bits/stdc++.h>
#include "wal_record.hpp"

class Transaction {
private:
    uint64_t txn_id;
    vector<WALRecord> operations;
    
public:
    Transaction(uint64_t txn_id);

    uint64_t getId() const;

    void addOperation(const WALRecord& record);

    const vector<WALRecord>& getOperations() const;
};