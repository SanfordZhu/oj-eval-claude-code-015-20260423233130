#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <list>
#include <unordered_map>

using namespace std;

const int NUM_BUCKETS = 19;
const int MAX_CACHE_SIZE = 5;
const string BUCKET_PREFIX = "bucket_";

struct CachedBucket {
    int bucket_num;
    vector<pair<string, pair<bool, int>>> entries;  // (index, (deleted, value))
    long file_size;
};

list<CachedBucket> cache_list;
unordered_map<int, list<CachedBucket>::iterator> cache_map;

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

void load_bucket(int bucket, CachedBucket &cb) {
    cb.bucket_num = bucket;
    cb.entries.clear();
    cb.file_size = 0;
    string filename = BUCKET_PREFIX + to_string(bucket) + ".dat";
    FILE *f = fopen(filename.c_str(), "rb");
    if (!f) {
        cb.file_size = 0;
        return;
    }

    fseek(f, 0, SEEK_END);
    cb.file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Estimate number of entries and reserve
    size_t estimated_entries = static_cast<size_t>(cb.file_size / (1 + sizeof(size_t) + 8 + sizeof(int)));
    cb.entries.reserve(estimated_entries);

    while (true) {
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
        cb.entries.push_back({idx, {static_cast<bool>(deleted), value}});
    }

    fclose(f);
}

CachedBucket &get_bucket_cache(int bucket) {
    auto it = cache_map.find(bucket);
    if (it != cache_map.end()) {
        // Move to front (MRU)
        cache_list.splice(cache_list.begin(), cache_list, it->second);
        return *it->second;
    }

    // Evict if full
    if (cache_list.size() >= MAX_CACHE_SIZE) {
        CachedBucket &last = cache_list.back();
        cache_map.erase(last.bucket_num);
        cache_list.pop_back();
    }

    // Load new bucket
    cache_list.emplace_front();
    CachedBucket &new_cb = cache_list.front();
    load_bucket(bucket, new_cb);
    cache_map[bucket] = cache_list.begin();
    return new_cb;
}

vector<int> find_entries(const string &index) {
    int b = get_bucket(index);
    CachedBucket &cb = get_bucket_cache(b);
    vector<int> result;

    for (const auto &entry : cb.entries) {
        if (!entry.second.first && entry.first == index) {
            result.push_back(entry.second.second);
        }
    }

    sort(result.begin(), result.end());
    return result;
}

void insert_entry(const string &index, int value) {
    int b = get_bucket(index);
    CachedBucket &cb = get_bucket_cache(b);

    // Check for duplicate
    for (const auto &entry : cb.entries) {
        if (!entry.second.first && entry.first == index && entry.second.second == value) {
            return;
        }
    }

    // Append to file
    string filename = BUCKET_PREFIX + to_string(b) + ".dat";
    FILE *f = fopen(filename.c_str(), "ab");
    uint8_t deleted = 0;
    fwrite(&deleted, 1, 1, f);
    size_t index_len = index.size();
    fwrite(&index_len, sizeof(index_len), 1, f);
    fwrite(index.data(), 1, index_len, f);
    fwrite(&value, sizeof(value), 1, f);
    fclose(f);

    // Add to cache
    cb.entries.push_back({index, {false, value}});
}

void delete_entry(const string &index, int value) {
    int b = get_bucket(index);
    CachedBucket &cb = get_bucket_cache(b);

    // Find in cache and mark as deleted
    bool found = false;
    for (auto &entry : cb.entries) {
        if (!entry.second.first && entry.first == index && entry.second.second == value) {
            entry.second.first = true;
            found = true;
            break;
        }
    }

    if (!found) {
        return;
    }

    // Update file: find the entry position and mark deleted
    string filename = BUCKET_PREFIX + to_string(b) + ".dat";
    FILE *f = fopen(filename.c_str(), "r+b");
    if (!f) {
        fclose(f);
        return;
    }

    long offset = 0;
    while (true) {
        long entry_offset = offset;
        uint8_t deleted;
        if (fread(&deleted, 1, 1, f) != 1) break;
        offset += 1;
        size_t index_len;
        if (fread(&index_len, sizeof(index_len), 1, f) != 1) break;
        offset += sizeof(index_len);
        string idx;
        idx.resize(index_len);
        if (fread(&idx[0], 1, index_len, f) != index_len) break;
        offset += index_len;
        int val;
        if (fread(&val, sizeof(val), 1, f) != 1) break;
        offset += sizeof(val);

        if (!deleted && idx == index && val == value) {
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
