#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

using namespace std;

const int NUM_BUCKETS = 19;
const string BUCKET_PREFIX = "bucket_";

uint64_t hash_string(const string &s) {
    const uint64_t FNV_OFFSET = 14695981039346313805ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;
    uint64_t hash = FNV_OFFSET;
    for (char c : s) {
        hash ^= static_cast<unsigned char>(c);
        hash *= FNV_PRIME;
    }
    return hash;
}

struct Entry {
    bool deleted;
    string index;
    int value;
};

int get_bucket(const string &index) {
    uint64_t h = hash_string(index);
    return static_cast<int>(h % NUM_BUCKETS);
}

vector<Entry> load_bucket(int bucket) {
    vector<Entry> entries;
    string filename = BUCKET_PREFIX + to_string(bucket) + ".dat";
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        return entries;
    }

    while (true) {
        uint8_t deleted_byte;
        if (fread(&deleted_byte, 1, 1, f) != 1) {
            break;
        }
        size_t index_len;
        if (fread(&index_len, sizeof(index_len), 1, f) != 1) {
            break;
        }
        string index;
        index.resize(index_len);
        if (fread(&index[0], 1, index_len, f) != index_len) {
            break;
        }
        int value;
        if (fread(&value, sizeof(value), 1, f) != 1) {
            break;
        }
        entries.push_back({static_cast<bool>(deleted_byte), index, value});
    }

    fclose(f);
    return entries;
}

void append_entry(int bucket, const Entry &entry) {
    string filename = BUCKET_PREFIX + to_string(bucket) + ".dat";
    FILE *f = fopen(filename.c_str(), "ab");
    if (!f) {
        return;
    }
    uint8_t deleted_byte = static_cast<uint8_t>(entry.deleted);
    fwrite(&deleted_byte, 1, 1, f);
    size_t index_len = entry.index.size();
    fwrite(&index_len, sizeof(index_len), 1, f);
    fwrite(entry.index.data(), 1, index_len, f);
    fwrite(&entry.value, sizeof(entry.value), 1, f);
    fclose(f);
}

void rewrite_bucket(int bucket, const vector<Entry> &entries) {
    string filename = BUCKET_PREFIX + to_string(bucket) + ".dat";
    FILE *f = fopen(filename.c_str(), "wb");
    if (!f) {
        return;
    }
    for (const auto &entry : entries) {
        uint8_t deleted_byte = static_cast<uint8_t>(entry.deleted);
        fwrite(&deleted_byte, 1, 1, f);
        size_t index_len = entry.index.size();
        fwrite(&index_len, sizeof(index_len), 1, f);
        fwrite(entry.index.data(), 1, index_len, f);
        fwrite(&entry.value, sizeof(entry.value), 1, f);
    }
    fclose(f);
}

void insert_entry(const string &index, int value) {
    int b = get_bucket(index);
    vector<Entry> entries = load_bucket(b);

    // Check if already exists
    for (const auto &e : entries) {
        if (!e.deleted && e.index == index && e.value == value) {
            return;
        }
    }

    // Append new entry - O(1) write
    append_entry(b, {false, index, value});
}

void delete_entry(const string &index, int value) {
    int b = get_bucket(index);
    vector<Entry> entries = load_bucket(b);

    bool found = false;
    for (auto &e : entries) {
        if (!e.deleted && e.index == index && e.value == value) {
            e.deleted = true;
            found = true;
            break;
        }
    }

    if (found) {
        rewrite_bucket(b, entries);
    }
}

vector<int> find_entries(const string &index) {
    int b = get_bucket(index);
    vector<Entry> entries = load_bucket(b);
    vector<int> result;

    for (const auto &e : entries) {
        if (!e.deleted && e.index == index) {
            result.push_back(e.value);
        }
    }

    sort(result.begin(), result.end());
    return result;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    for (int i = 0; i < n; ++i) {
        string cmd;
        cin >> cmd;

        if (cmd == "insert") {
            string index;
            int value;
            cin >> index >> value;
            insert_entry(index, value);
        } else if (cmd == "delete") {
            string index;
            int value;
            cin >> index >> value;
            delete_entry(index, value);
        } else if (cmd == "find") {
            string index;
            cin >> index;
            vector<int> result = find_entries(index);
            if (result.empty()) {
                cout << "null\n";
            } else {
                for (size_t j = 0; j < result.size(); ++j) {
                    if (j > 0) {
                        cout << ' ';
                    }
                    cout << result[j];
                }
                cout << '\n';
            }
        }
    }

    return 0;
}
