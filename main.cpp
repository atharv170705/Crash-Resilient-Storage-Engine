#include <bits/stdc++.h>
#include "database.hpp"

using namespace std;

int main() {

    remove("data/database.wal");

    // First process.
    {
        Database db;

        db.begin();
        db.set("A", "100");
        db.commit();
    }

    // Simulate a restart.
    {
        Database db;

        string value;

        if (db.get("A", value)) {
            cout << "After restart: A = " << value << "\n";
        }
        else {
            cout << "A does not exist\n";
        }

        db.begin();
        db.set("B", "200");
        db.commit();
    }

    // Another restart.
    {
        Database db;

        string value;

        if (db.get("A", value)) {
            cout << "After second restart: A = " << value << "\n";
        }

        if (db.get("B", value)) {
            cout << "After second restart: B = " << value << "\n";
        }
    }

    return 0;
}