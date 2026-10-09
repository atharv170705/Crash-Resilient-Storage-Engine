#pragma once

#include <bits/stdc++.h>
#include "wal_reader.hpp"
#include "storage_engine.hpp"

using namespace std;

class RecoveryManager {
private:
    string filename;
    WALReader reader;
    
    void truncateWAL(uint64_t validBytes);

public:
    RecoveryManager(const string& walFilename);

    uint64_t recover(StorageEngine& storage);
};