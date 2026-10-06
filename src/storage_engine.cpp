#include <bits/stdc++.h>
#include "storage_engine.hpp"

using namespace std;

void StorageEngine::set(const string& key, const string& value) {
    data[key] = value;
}

bool StorageEngine::get(const string& key, string& value) const {
    auto it = data.find(key);
    if(it == data.end()) {
        return false;
    }
    value = it -> second;
    return true;
}

bool StorageEngine::remove(const string& key) {
    return data.erase(key) > 0;
}

void StorageEngine::save() const {
    ofstream file(filename); // open file for writing
    if(!file.is_open()) {
        cerr << "Failed to open database file\n";
        return;
    }
    for(const auto& [key, value] : data) {
        file << key << ' ' << value << '\n';
    }
};

void StorageEngine::load() {
    ifstream file(filename); // open file for reading
    if(!file.is_open()) {
        return;
    }
    string key;
    string value;

    while(file >> key >> value) {
        data[key] = value;
    }
}