#pragma once

#include "storage_engine.hpp"

using namespace std;

class Database {
private:
    StorageEngine storage;    
public:
    void set(const string& key, const string& value);
    bool get(const string& key, string& value) const;
    bool remove(const string &key);

    void save() const;
    void load();
};