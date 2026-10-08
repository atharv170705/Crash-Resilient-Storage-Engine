#pragma once

#include <bits/stdc++.h>

#include "storage_engine.hpp"
#include "wal_manager.hpp"
#include "transaction.hpp"

using namespace std;

class Database {
private:
    StorageEngine storage;
    WALManager wal;

    unique_ptr<Transaction> activeTransaction;

    uint64_t nextTxnId;   
public:
    Database();

    void set(const string& key, const string& value);
    bool get(const string& key, string& value) const;
    bool remove(const string &key);

    void begin();
    void commit();
    void rollback();

    void save() const;
    void load();
};