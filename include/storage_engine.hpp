#pragma once

#include<string>
#include <unordered_map>

using namespace std;

class StorageEngine {
private:
    unordered_map<string, string> data;
    string filename = "data/database.txt";
public:
    void set(const string& key, const string& value);
    
    bool get(const string& key, string& value) const;

    bool remove(const string &key);

    void save() const;
    void load();
};