#include "colpool.hpp"

uint64_t hashSequence(const std::vector<int>& seq) {
    uint64_t h = 0x84222325cbf29ce4ULL ^ seq.size();
    for (int f : seq) {
        h ^= static_cast<uint64_t>(f) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 32;
    }
    return h;
}

int ColumnPool::add(const std::vector<int>& seq, long long cost, int source, long long stamp, bool* isNew) {
    const uint64_t h = hashSequence(seq);
    auto it = index_.find(h);
    if (it != index_.end()) {
        Column& c = cols_[it->second];
        if (isNew) *isNew = false;
        if (c.seq != seq) return -1;  // 64-bit hash collision: ignore the newcomer
        ++c.hits;
        c.lastSeen = stamp;
        return it->second;
    }
    Column c;
    c.seq = seq;
    c.cost = cost;
    c.hash = h;
    c.source = source;
    c.hits = 1;
    c.lastSeen = stamp;
    cols_.push_back(std::move(c));
    const int id = static_cast<int>(cols_.size()) - 1;
    index_.emplace(h, id);
    if (isNew) *isNew = true;
    return id;
}

int ColumnPool::find(const std::vector<int>& seq) const {
    auto it = index_.find(hashSequence(seq));
    if (it == index_.end() || cols_[it->second].seq != seq) return -1;
    return it->second;
}

void ColumnPool::retain(const std::vector<char>& keep) {
    std::vector<Column> kept;
    kept.reserve(cols_.size());
    for (size_t i = 0; i < cols_.size(); ++i)
        if (keep[i]) kept.push_back(std::move(cols_[i]));
    cols_ = std::move(kept);
    index_.clear();
    for (size_t i = 0; i < cols_.size(); ++i) index_.emplace(cols_[i].hash, static_cast<int>(i));
}

void ColumnPool::clear() {
    cols_.clear();
    index_.clear();
}
