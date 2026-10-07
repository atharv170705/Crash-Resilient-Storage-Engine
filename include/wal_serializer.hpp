#pragma once

#include <bits/stdc++.h>

#include "wal_record.hpp"

class WALSerializer {
private:
    static void appendUint16(vector<uint8_t>& buffer, uint16_t value);
    static void appendUint32(vector<uint8_t>& buffer, uint32_t value);
    static void appendUint64(vector<uint8_t>& buffer, uint64_t value);
    static void appendString(vector<uint8_t>& buffer, const string& value);

    static uint16_t readUint16(const vector<uint8_t>& buffer, size_t& offset);
    static uint32_t readUint32(const vector<uint8_t>& buffer, size_t& offset);
    static uint64_t readUint64(const vector<uint8_t>& buffer, size_t& offset);
    static string readString(const vector<uint8_t>& buffer, size_t& offset);

public:
    static vector<uint8_t> serialize(const WALRecord& record);
    
    static bool deserialize(const vector<uint8_t>& buffer, WALRecord& record);    
};