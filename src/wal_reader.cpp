#include "wal_reader.hpp"
#include "wal_serializer.hpp"

using namespace std;

WALReader::WALReader(const string& filename) 
    : filename(filename) {

}

vector<WALRecord> WALReader::readAll() {
    vector<WALRecord> records;
    ifstream file(filename, ios::binary);
    if(!file.is_open()) {
        throw runtime_error("Failed to open WAL file");
    }

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
            break;
        }

        uint32_t recordLength = 0;
        for(int i = 0; i < 4; i++) {
            recordLength |= static_cast<uint32_t>(header[6 + i]) << (8 * i);
        }

        if (recordLength < 10) {
            cerr << "Invalid WAL record length\n";
            break;
        }

        vector<uint8_t> buffer(recordLength);

        for (int i = 0; i < 10; i++) {
            buffer[i] = header[i];
        }

        file.read(reinterpret_cast<char*>(buffer.data() + 10), recordLength - 10);

        if (file.gcount() != static_cast<streamsize>(recordLength - 10)) {
            cerr << "Incomplete WAL record\n";
            break;
        }

        WALRecord record;

        if (!WALSerializer::deserialize(buffer, record)) {
            cerr << "Invalid WAL record detected\n";
            break;
        }

        records.push_back(record);
    }

    return records;
}