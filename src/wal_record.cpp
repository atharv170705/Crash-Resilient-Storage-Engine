#include "wal_record.hpp"

using namespace std;

WALRecord::WALRecord()
    : txn_id(0), op(OpCode::BEGIN) {
}

WALRecord::WALRecord(uint64_t txn_id, OpCode op, const string& key, const string& value)
    : txn_id(txn_id), op(op), key(key), value(value) {
}