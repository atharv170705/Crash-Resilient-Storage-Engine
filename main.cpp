#include <bits/stdc++.h>
#include "database.hpp"
#include <signal.h>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Usage: ./minidb crash | verify | tail-test | verify-tail\n";
        return 1;
    }

    string mode = argv[1];

    if (mode == "crash") {
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

    if (mode == "verify") {
        Database db;

        string valueA, valueB;

        bool hasA = db.get("A", valueA);
        bool hasB = db.get("B", valueB);

        cout << "A: " << (hasA ? valueA : "missing") << "\n";
        cout << "B: " << (hasB ? valueB : "missing") << "\n";

        bool passed = hasA && valueA == "100" && !hasB;

        cout << "\nCrash recovery test: "
             << (passed ? "PASSED" : "FAILED") << "\n";

        return passed ? 0 : 1;
    }

    if (mode == "tail-test") {
        remove("data/database.wal");

        // Create a valid committed transaction.
        {
            Database db;

            db.begin();
            db.set("A", "100");
            db.commit();
        }

        // Append deliberately corrupted bytes.
        {
            ofstream file("data/database.wal", ios::binary | ios::app);

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

        // Startup recovery must truncate the garbage.
        {
            Database db;

            string value;

            if (!db.get("A", value) || value != "100") {
                cerr << "Recovery failed to preserve A\n";
                return 1;
            }

            // This transaction must be appended after the repaired WAL.
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

        cout << "\nWAL tail repair test: "
             << (passed ? "PASSED" : "FAILED") << "\n";

        return passed ? 0 : 1;
    }

    cout << "Unknown mode.\n";
    return 1;
}