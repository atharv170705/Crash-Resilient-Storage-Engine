#pragma once

#include <bits/stdc++.h>
#include "wal_record.hpp"

using namespace std;

class WALReader {
private:
    string filename;

public:
    WALReader(const string& filename);

    vector<WALRecord> readAll(uint64_t& validBytes, bool& invalidTail);
};