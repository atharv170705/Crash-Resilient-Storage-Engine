#include <bits/stdc++.h>
#include "database.hpp"
#include <signal.h>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: ./minidb crash-test | verify-crash | tail-test | verify-tail | crc-test | verify-crc\n";
        return 1;
    }

    string mode = argv[1];

    if (mode == "crash-test") {
        remove("data/database.wal");

        Database db;

        db.begin();
        db.set("A", "100");
        db.commit();

        db.begin();
        db.set("B", "200");

        cout << "A committed. B is uncommitted.\n";
        cout << "Killing process with SIGKILL...\n" << flush;

        kill(getpid(), SIGKILL);

        return 0;
    }

    if (mode == "verify-crash") {
        Database db;

        string valueA, valueB;

        bool hasA = db.get("A", valueA);
        bool hasB = db.get("B", valueB);

        cout << "A: " << (hasA ? valueA : "missing") << "\n";
        cout << "B: " << (hasB ? valueB : "missing") << "\n";

        bool passed = hasA && valueA == "100" && !hasB;

        cout << "\nCrash recovery test: " << (passed ? "PASSED" : "FAILED") << "\n";

        return passed ? 0 : 1;
    }

    if (mode == "tail-test") {
        remove("data/database.wal");

        {
            Database db;

            db.begin();
            db.set("A", "100");
            db.commit();
        }

        {
            ofstream file(
                "data/database.wal",
                ios::binary | ios::app
            );

            if (!file.is_open()) {
                cerr << "Failed to open WAL for corruption test\n";
                return 1;
            }

            const char garbage[] = "BAD";

            file.write(garbage, sizeof(garbage));
            file.flush();

            if (!file) {
                cerr << "Failed to corrupt WAL for test\n";
                return 1;
            }
        }

        cout << "Appended garbage to WAL.\n";

        {
            Database db;

            string value;

            if (!db.get("A", value) || value != "100") {
                cerr << "Recovery failed to preserve A\n";
                return 1;
            }

            db.begin();
            db.set("B", "200");
            db.commit();
        }

        cout << "Committed B after WAL repair.\n";
        cout << "Run ./minidb verify-tail to verify another restart.\n";

        return 0;
    }

    if (mode == "verify-tail") {
        Database db;

        string valueA, valueB;

        bool hasA = db.get("A", valueA);
        bool hasB = db.get("B", valueB);

        cout << "A: " << (hasA ? valueA : "missing") << "\n";
        cout << "B: " << (hasB ? valueB : "missing") << "\n";

        bool passed =
            hasA && valueA == "100" &&
            hasB && valueB == "200";

        cout << "\nWAL tail repair test: " << (passed ? "PASSED" : "FAILED") << "\n";

        return passed ? 0 : 1;
    }

    if (mode == "crc-test") {
        remove("data/database.wal");

        // Create two committed transactions.
        {
            Database db;

            db.begin();
            db.set("A", "100");
            db.commit();

            db.begin();
            db.set("B", "200");
            db.commit();
        }

        // Corrupt the final byte of the last record's CRC32.
        {
            fstream file(
                "data/database.wal",
                ios::binary | ios::in | ios::out
            );

            if (!file.is_open()) {
                cerr << "Failed to open WAL for CRC test\n";
                return 1;
            }

            file.seekg(-1, ios::end);

            char byte;

            if (!file.get(byte)) {
                cerr << "Failed to read WAL byte\n";
                return 1;
            }

            byte ^= 0xFF;

            file.seekp(-1, ios::end);
            file.put(byte);
            file.flush();

            if (!file) {
                cerr << "Failed to corrupt CRC32\n";
                return 1;
            }
        }

        cout << "Corrupted the final WAL record's CRC32.\n";

        // Startup recovery should reject and truncate the record.
        {
            Database db;

            string value;

            if (!db.get("A", value) || value != "100") {
                cerr << "Recovery failed to preserve A\n";
                return 1;
            }

            if (db.get("B", value)) {
                cerr << "Corrupted transaction B was recovered\n";
                return 1;
            }
        }

        cout << "CRC validation and recovery completed.\n";
        cout << "Run ./minidb verify-crc to verify another restart.\n";

        return 0;
    }

    if (mode == "verify-crc") {
        Database db;

        string valueA, valueB;

        bool hasA = db.get("A", valueA);
        bool hasB = db.get("B", valueB);

        cout << "A: " << (hasA ? valueA : "missing") << "\n";
        cout << "B: " << (hasB ? valueB : "missing") << "\n";

        bool passed =
            hasA && valueA == "100" && !hasB;

        cout << "\nCRC corruption test: " << (passed ? "PASSED" : "FAILED") << "\n";

        return passed ? 0 : 1;
    }

    cout << "Unknown mode.\n";
    return 1;
}