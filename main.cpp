#include <bits/stdc++.h>
#include "database.hpp"

#include "wal_manager.hpp"
#include "wal_serializer.hpp"

using namespace std;

int main() {
    Database db;

    // Create a committed value first.
    db.begin();
    db.set("A", "50");
    db.commit();

    string value;

    // Modify and delete the committed value inside a transaction.
    db.begin();

    db.set("A", "100");
    cout << "After SET A=100: ";

    if (db.get("A", value)) {
        cout << "A = " << value << '\n';
    }

    db.remove("A");

    cout << "After DELETE A: ";

    if (db.get("A", value)) {
        cout << "A = " << value << '\n';
    }
    else {
        cout << "A not found\n";
    }

    db.rollback();

    // Rollback should restore the previously committed value.
    cout << "After rollback: ";

    if (db.get("A", value)) {
        cout << "A = " << value << '\n';
    }
    else {
        cout << "A not found\n";
    }

    // Test deleting a key created only inside the transaction.
    db.begin();

    db.set("B", "200");
    db.remove("B");

    cout << "New key B after SET + DELETE: ";

    if (db.get("B", value)) {
        cout << "B = " << value << '\n';
    }
    else {
        cout << "B not found\n";
    }

    db.rollback();


    return 0;
}