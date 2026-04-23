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

int get_bucket(const string &index) {
    uint64_t h = hash_string(index);
    return static_cast<int>(h % NUM_BUCKETS);
}

vector<int> find_entries(const string &index) {
    int b = get_bucket(index);
    string filename = BUCKET_PREFIX + to_string(b) + ".dat";
    FILE *f = fopen(filename.c_str(), "rb");
    vector<int> result;

    if (!f) {
        return result;
    }

    while (true) {
        long entry_offset = ftell(f);
        uint8_t deleted;
        if (fread(&deleted, 1, 1, f) != 1) {
            break;
        }
        size_t index_len;
        if (fread(&index_len, sizeof(index_len), 1, f) != 1) {
            break;
        }
        string idx;
        idx.resize(index_len);
        if (fread(&idx[0], 1, index_len, f) != index_len) {
            break;
        }
        int value;
        if (fread(&value, sizeof(value), 1, f) != 1) {
            break;
        }

        if (!deleted && idx == index) {
            result.push_back(value);
        }
    }

    fclose(f);
    sort(result.begin(), result.end());
    return result;
}

void insert_entry(const string &index, int value) {
    int b = get_bucket(index);
    string filename = BUCKET_PREFIX + to_string(b) + ".dat";

    // Check for duplicate - need to scan entire file
    FILE *check = fopen(filename.c_str(), "rb");
    if (check) {
        bool exists = false;
        while (true) {
            uint8_t deleted;
            if (fread(&deleted, 1, 1, check) != 1) {
                break;
            }
            size_t index_len;
            if (fread(&index_len, sizeof(index_len), 1, check) != 1) {
                break;
            }
            string idx;
            idx.resize(index_len);
            if (fread(&idx[0], 1, index_len, check) != index_len) {
                break;
            }
            int val;
            if (fread(&val, sizeof(val), 1, check) != 1) {
                break;
            }
            if (!deleted && idx == index && val == value) {
                exists = true;
                break;
            }
        }
        fclose(check);
        if (exists) {
            return;
        }
    }

    // Append new entry
    FILE *f = fopen(filename.c_str(), "ab");
    if (!f) {
        return;
    }
    uint8_t deleted = 0;
    fwrite(&deleted, 1, 1, f);
    size_t index_len = index.size();
    fwrite(&index_len, sizeof(index_len), 1, f);
    fwrite(index.data(), 1, index_len, f);
    fwrite(&value, sizeof(value), 1, f);
    fclose(f);
}

void delete_entry(const string &index, int value) {
    int b = get_bucket(index);
    string filename = BUCKET_PREFIX + to_string(b) + ".dat";

    FILE *f = fopen(filename.c_str(), "r+b");
    if (!f) {
        return;
    }

    while (true) {
        long entry_offset = ftell(f);
        uint8_t deleted;
        if (fread(&deleted, 1, 1, f) != 1) {
            break;
        }
        size_t index_len;
        if (fread(&index_len, sizeof(index_len), 1, f) != 1) {
            break;
        }
        string idx;
        idx.resize(index_len);
        if (fread(&idx[0], 1, index_len, f) != index_len) {
            break;
        }
        int val;
        if (fread(&val, sizeof(val), 1, f) != 1) {
            break;
        }

        if (!deleted && idx == index && val == value) {
            // Seek back to the deleted flag byte and set it to 1
            fseek(f, entry_offset, SEEK_SET);
            uint8_t new_deleted = 1;
            fwrite(&new_deleted, 1, 1, f);
            fflush(f);
            break;
        }
    }

    fclose(f);
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
