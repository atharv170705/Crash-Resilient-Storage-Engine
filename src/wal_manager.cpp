#include "wal_manager.hpp"
#include "wal_serializer.hpp"

#include <fcntl.h>
#include <unistd.h>


WALManager::WALManager(const string& filename)
    : filename(filename) {
}

void WALManager::append(const WALRecord& record) {
    vector<uint8_t> bytes = WALSerializer::serialize(record);

    // ofstream file(filename, ios::binary | ios::app);
    // // ios::binary - Open the file in binary mode
    // // ios::app - Open in append mode; writes go to the end of the file

    // if (!file.is_open()) {
    //     throw runtime_error("Failed to open WAL file");
    // }

    // file.write(
    //     reinterpret_cast<const char*>(bytes.data()),
    //     bytes.size()
    // );


    int fd = open(
        filename.c_str(),
        O_WRONLY | O_CREAT | O_APPEND,
        0644
    );

    /*
    O_WRONLY  → open for writing
    O_CREAT   → create file if it doesn't exist
    O_APPEND  → always write at the end
    */

    if (fd == -1) {
        throw runtime_error("Failed to open WAL file");
    }

    ssize_t bytesWritten = write(fd, bytes.data(), bytes.size());

    if (bytesWritten != static_cast<ssize_t>(bytes.size())) {
        close(fd);
        throw runtime_error("Failed to write complete WAL record");
    }

    if (fsync(fd) == -1) {
        close(fd);
        throw runtime_error("Failed to fsync WAL");
    }

    close(fd);
}