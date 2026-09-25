// Pool of distinct runway sequences (columns) with their costs.
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

struct Column {
    std::vector<int> seq;
    long long cost = 0;
    uint64_t hash = 0;
    int source = 0;     // 0 = own ILS, 1 = external pool file, 2 = SP/derived
    int hits = 0;       // how many times it was (re)submitted
    long long lastSeen = 0;
};

uint64_t hashSequence(const std::vector<int>& seq);

class ColumnPool {
public:
    // Returns the column id, inserting it if new (isNew reports which).
    int add(const std::vector<int>& seq, long long cost, int source, long long stamp, bool* isNew = nullptr);
    int find(const std::vector<int>& seq) const;
    size_t size() const { return cols_.size(); }
    const Column& col(size_t i) const { return cols_[i]; }
    // Drops columns not in `keep` (ids are renumbered).
    void retain(const std::vector<char>& keep);
    void clear();

private:
    std::vector<Column> cols_;
    std::unordered_map<uint64_t, int> index_;
};
