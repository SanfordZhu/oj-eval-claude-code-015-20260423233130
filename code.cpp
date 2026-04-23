#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <iomanip>

using namespace std;

const int NUM_BUCKETS = 16;
const string BUCKET_PREFIX = "bucket_";

uint64_t hash_string(const string &s) {
    uint64_t hash = 0;
    for (char c : s) {
        hash = hash * 31 + static_cast<unsigned char>(c);
    }
    return hash;
}

struct Entry {
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
    ifstream in(filename, ios::binary);
    if (!in) {
        return entries;
    }

    while (in.peek() != EOF) {
        size_t index_len;
        if (!in.read(reinterpret_cast<char*>(&index_len), sizeof(index_len))) {
            break;
        }
        string index;
        index.resize(index_len);
        if (!in.read(&index[0], index_len)) {
            break;
        }
        int value;
        if (!in.read(reinterpret_cast<char*>(&value), sizeof(value))) {
            break;
        }
        entries.push_back({index, value});
    }

    in.close();
    return entries;
}

void save_bucket(int bucket, const vector<Entry> &entries) {
    string filename = BUCKET_PREFIX + to_string(bucket) + ".dat";
    ofstream out(filename, ios::binary | ios::trunc);
    if (!out) {
        cerr << "Failed to write bucket " << bucket << endl;
        return;
    }

    for (const auto &entry : entries) {
        size_t index_len = entry.index.size();
        out.write(reinterpret_cast<const char*>(&index_len), sizeof(index_len));
        out.write(entry.index.data(), entry.index.size());
        out.write(reinterpret_cast<const char*>(&entry.value), sizeof(entry.value));
    }

    out.close();
}

void insert_entry(const string &index, int value) {
    int b = get_bucket(index);
    vector<Entry> entries = load_bucket(b);

    auto it = lower_bound(entries.begin(), entries.end(), Entry{index, value},
        [](const Entry &a, const Entry &b) {
            if (a.index != b.index) {
                return a.index < b.index;
            }
            return a.value < b.value;
        });

    if (it != entries.end() && it->index == index && it->value == value) {
        return;
    }

    entries.insert(it, Entry{index, value});
    save_bucket(b, entries);
}

void delete_entry(const string &index, int value) {
    int b = get_bucket(index);
    vector<Entry> entries = load_bucket(b);

    auto it = lower_bound(entries.begin(), entries.end(), Entry{index, value},
        [](const Entry &a, const Entry &b) {
            if (a.index != b.index) {
                return a.index < b.index;
            }
            return a.value < b.value;
        });

    if (it != entries.end() && it->index == index && it->value == value) {
        entries.erase(it);
        save_bucket(b, entries);
    }
}

vector<int> find_entries(const string &index) {
    int b = get_bucket(index);
    vector<Entry> entries = load_bucket(b);
    vector<int> result;

    auto comp = [](const Entry &entry, const string &idx) {
        return entry.index < idx;
    };

    auto it = lower_bound(entries.begin(), entries.end(), index, comp);

    while (it != entries.end() && it->index == index) {
        result.push_back(it->value);
        ++it;
    }

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
