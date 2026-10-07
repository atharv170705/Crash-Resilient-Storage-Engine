#pragma once

#include <bits/stdc++.h>

using namespace std;

enum class OpCode : uint8_t {
    BEGIN = 1,
    SET = 2,
    DELETE_KEY = 3,
    COMMIT = 4,
    ROLLBACK = 5
};

class WALRecord {
public:
    static constexpr uint32_t MAGIC = 0x57414C31;
    static constexpr uint16_t VERSION = 1;

    uint64_t txn_id;
    OpCode op;

    string key;
    string value;

    WALRecord();

    WALRecord(uint64_t txn_id, OpCode op, const string& key = "", const string& value = "");
};