#include "database.hpp"

using namespace std;

void Database::set(const string& key, const string& value) {
    storage.set(key, value);
}

bool Database::get(const string& key, string& value) const {
    return storage.get(key, value);
}

bool Database::remove(const string& key) {
    return storage.remove(key);
}

void Database::save() const {
    storage.save();
}

void Database::load() {
    storage.load();
}