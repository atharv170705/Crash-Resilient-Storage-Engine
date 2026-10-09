#include "wal_reader.hpp"
#include "wal_serializer.hpp"

using namespace std;

WALReader::WALReader(const string& filename) 
    : filename(filename) {

}

vector<WALRecord> WALReader::readAll(uint64_t& validBytes, bool& invalidTail) {
    vector<WALRecord> records;
    validBytes = 0;
    invalidTail = false;
    ifstream file(filename, ios::binary);
    if(!file.is_open()) {
        return records;
    }

    const uint32_t MAX_RECORD_SIZE = 64 * 1024 * 1024;

    while(true) {
        //Read MAGIC + VERSION + LENGTH = 4 + 2 + 4 = 10 bytes.
        uint8_t header[10];
        file.read(reinterpret_cast<char*>(header), sizeof(header));

        streamsize bytesRead = file.gcount();

        if (bytesRead == 0) {
            break;
        }

        if (bytesRead != sizeof(header)) {
            cerr << "Incomplete WAL header\n";
            invalidTail = true;
            break;
        }

        uint32_t recordLength = 0;
        for(int i = 0; i < 4; i++) {
            recordLength |= static_cast<uint32_t>(header[6 + i]) << (8 * i);
        }

        if (recordLength < 23 || recordLength > MAX_RECORD_SIZE) {
            cerr << "Invalid WAL record length\n";
            invalidTail = true;
            break;
        }

        vector<uint8_t> buffer(recordLength);

        for (int i = 0; i < 10; i++) {
            buffer[i] = header[i];
        }

        file.read(reinterpret_cast<char*>(buffer.data() + 10), recordLength - 10);

        if (file.gcount() != static_cast<streamsize>(recordLength - 10)) {
            cerr << "Incomplete WAL record\n";
            invalidTail = true;
            break;
        }

        WALRecord record;

        if (!WALSerializer::deserialize(buffer, record)) {
            cerr << "Invalid WAL record detected\n";
            invalidTail = true;
            break;
        }

        records.push_back(record);
        validBytes += recordLength;
    }

    return records;
}