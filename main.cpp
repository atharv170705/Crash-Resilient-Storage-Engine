#include <bits/stdc++.h>
#include "database.hpp"

#include "wal_manager.hpp"
#include "wal_serializer.hpp"

using namespace std;

int main() {
    Database db;

    // Start a transaction
    db.begin();

    // Write inside transaction
    db.set("A", "100");

    // GET should see the uncommitted value
    string value;

    if (db.get("A", value)) {
        cout << "Inside transaction: A = " << value << '\n';
    }
    else {
        cout << "Inside transaction: A not found\n";
    }

    // Rollback the transaction
    db.rollback();

    // GET after rollback
    if (db.get("A", value)) {
        cout << "After rollback: A = " << value << '\n';
    }
    else {
        cout << "After rollback: A not found\n";
    }

    return 0;
}