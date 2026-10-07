#pragma once

#include <bits/stdc++.h>

#include "wal_record.hpp"

using namespace std;

class WALManager {
private:
    string filename;

public:
    WALManager(const string& filename);

    void append(const WALRecord& record);
};