#include "wal_manager.hpp"
#include "wal_serializer.hpp"

WALManager::WALManager(const string& filename)
    : filename(filename) {
}

void WALManager::append(const WALRecord& record) {
    vector<uint8_t> bytes = WALSerializer::serialize(record);

    ofstream file(filename, ios::binary | ios::app);
    // ios::binary - Open the file in binary mode
    // ios::app - Open in append mode; writes go to the end of the file

    if (!file.is_open()) {
        throw runtime_error("Failed to open WAL file");
    }

    file.write(
        reinterpret_cast<const char*>(bytes.data()),
        bytes.size()
    );
}