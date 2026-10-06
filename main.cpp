#include <bits/stdc++.h>
#include "database.hpp"

using namespace std;

int main() {
    Database db;
    db.load();

    string line;

    while(true) {
        cout << "> ";

        if(!getline(cin, line)) return 0;

        stringstream ss(line);
        string command;
        
        ss >> command;

        if(command == "SET") {
            string key;
            string value;

            ss >> key >> value;

            db.set(key, value);

            cout << "OK" << endl;
        }
        else if (command == "GET") {
            string key;
            ss >> key;

            string value;

            if (db.get(key, value)) {
                cout << value << endl;
            } else {
                cout << "NOT FOUND" << endl;
            }
        }
        else if (command == "DELETE") {
            string key;
            ss >> key;

            if (db.remove(key)) {
                cout << "OK" << endl;
            } else {
                cout << "NOT FOUND" << endl;
            }
        }
        else if (command == "EXIT") {
            db.save();
            break;
        }
        else {
            cout << "UNKNOWN COMMAND" << endl;
        }
    }

    return 0;
}