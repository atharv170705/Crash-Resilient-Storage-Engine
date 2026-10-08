#include <bits/stdc++.h>
#include "wal_serializer.hpp"
#include "crc32.hpp"

using namespace std;


// 0xFF -> 11111111 (8 set bits)
// like how we mask for 1 bit : (val >> i) & 1
// for 8 bits val >> (8 * i) & 0xFF
void WALSerializer::appendUint16(vector<uint8_t>& buffer, uint16_t value) {
    for (int i = 0; i < 2; i++) {
        buffer.push_back((value >> (8 * i)) & 0xFF);
    }
}

void WALSerializer::appendUint32(vector<uint8_t>& buffer, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        buffer.push_back((value >> (8 * i)) & 0xFF);
    }
}

void WALSerializer::appendUint64(vector<uint8_t>& buffer, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        buffer.push_back((value >> (8 * i)) & 0xFF);
    }
}

uint16_t WALSerializer::readUint16(const vector<uint8_t>& buffer, size_t& offset) {
    uint16_t value = 0;
    for (int i = 0; i < 2; i++) {
        value |= static_cast<uint32_t>(buffer[offset++]) << (8 * i);
    }
    return value;
}

uint32_t WALSerializer::readUint32(const vector<uint8_t>& buffer, size_t& offset) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= static_cast<uint32_t>(buffer[offset++]) << (8 * i);
    }
    return value;
}

uint64_t WALSerializer::readUint64(const vector<uint8_t>& buffer, size_t& offset) {
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= static_cast<uint64_t>(buffer[offset++]) << (8 * i);
    }
    return value;
}


void WALSerializer::appendString(vector<uint8_t>& buffer, const string& value) {
    appendUint32(buffer, value.size());

    for (char c : value) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

string WALSerializer::readString(const vector<uint8_t>& buffer, size_t& offset) {
    uint32_t length = readUint32(buffer, offset);

    string value;

    for (uint32_t i = 0; i < length; i++) {
        value.push_back(static_cast<char>(buffer[offset++]));
    }

    return value;
}



vector<uint8_t> WALSerializer::serialize(const WALRecord& record) {
    vector<uint8_t> buffer;

    // Header
    appendUint32(buffer, WALRecord::MAGIC);
    appendUint16(buffer, WALRecord::VERSION);

    // Reserve space for record length.
    size_t lengthOffset = buffer.size();
    appendUint32(buffer, 0);

    appendUint64(buffer, record.txn_id);

    buffer.push_back(static_cast<uint8_t>(record.op));

    // Payload
    if (record.op == OpCode::SET) {
        appendString(buffer, record.key);
        appendString(buffer, record.value);
    }
    else if (record.op == OpCode::DELETE_KEY) {
        appendString(buffer, record.key);
    }

    // Total record length includes CRC.
    uint32_t recordLength = buffer.size() + sizeof(uint32_t);

    // Fill record length.
    for (int i = 0; i < 4; i++) {
        buffer[lengthOffset + i] = (recordLength >> (8 * i)) & 0xFF;
    }

    // CRC covers everything before the CRC itself.
    uint32_t crc = CRC32::calculate(buffer);

    appendUint32(buffer, crc);

    return buffer;
}


bool WALSerializer::deserialize(const vector<uint8_t>& buffer, WALRecord& record) {
    if (buffer.size() < 23) {
        return false;
    }

    size_t offset = 0;

    uint32_t magic = readUint32(buffer, offset);

    if (magic != WALRecord::MAGIC) {
        return false;
    }

    uint16_t version = readUint16(buffer, offset);

    if (version != WALRecord::VERSION) {
        return false;
    }

    uint32_t recordLength = readUint32(buffer, offset);

    if (recordLength != buffer.size()) {
        return false;
    }

    uint64_t txnId = readUint64(buffer, offset);

    uint8_t opCode = buffer[offset++];

    if (opCode < static_cast<uint8_t>(OpCode::BEGIN) || opCode > static_cast<uint8_t>(OpCode::ROLLBACK)) {
        return false;
    }

    record.txn_id = txnId;
    record.op = static_cast<OpCode>(opCode);

    if (record.op == OpCode::SET) {
        record.key = readString(buffer, offset);
        record.value = readString(buffer, offset);
    }
    else if (record.op == OpCode::DELETE_KEY) {
        record.key = readString(buffer, offset);
    }

    uint32_t storedCRC = readUint32(buffer, offset);

    vector<uint8_t> data(buffer.begin(), buffer.begin() + buffer.size() - sizeof(uint32_t));

    uint32_t calculatedCRC = CRC32::calculate(data);

    if (storedCRC != calculatedCRC) {
        return false;
    }

    return offset == buffer.size();
}