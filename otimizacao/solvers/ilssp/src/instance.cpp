#include "instance.hpp"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace {

std::vector<long long> readAllInts(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open instance " + path);
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string data = ss.str();
    std::vector<long long> out;
    out.reserve(data.size() / 2);
    long long cur = 0;
    bool inNum = false, neg = false;
    for (char ch : data) {
        if (ch >= '0' && ch <= '9') {
            cur = cur * 10 + (ch - '0');
            inNum = true;
        } else {
            if (inNum) out.push_back(neg ? -cur : cur);
            else if (neg) throw std::runtime_error("stray '-' in " + path);
            cur = 0;
            inNum = false;
            neg = (ch == '-');
        }
    }
    if (inNum) out.push_back(neg ? -cur : cur);
    return out;
}

}  // namespace

Instance loadInstance(const std::string& path) {
    const std::vector<long long> v = readAllInts(path);
    if (v.size() < 2) throw std::runtime_error("instance too short: " + path);
    Instance ins;
    ins.n = static_cast<int>(v[0]);
    ins.m = static_cast<int>(v[1]);
    const size_t n = static_cast<size_t>(ins.n);
    if (ins.n <= 0 || ins.m <= 0) throw std::runtime_error("bad n/m in " + path);
    if (v.size() != 2 + 3 * n + n * n)
        throw std::runtime_error("bad instance size in " + path);
    ins.r.assign(v.begin() + 2, v.begin() + 2 + n);
    ins.c.assign(v.begin() + 2 + n, v.begin() + 2 + 2 * n);
    ins.p.assign(v.begin() + 2 + 2 * n, v.begin() + 2 + 3 * n);
    ins.t.assign(v.begin() + 2 + 3 * n, v.end());
    for (size_t i = 0; i < n; ++i) {
        if (ins.r[i] < 0 || ins.c[i] < 0 || ins.p[i] < 0)
            throw std::runtime_error("negative data in " + path);
        ins.maxC = std::max(ins.maxC, ins.c[i]);
    }
    for (int x : ins.t) {
        if (x < 0) throw std::runtime_error("negative separation in " + path);
        ins.maxT = std::max(ins.maxT, x);
    }
    ins.byRelease.resize(n);
    std::iota(ins.byRelease.begin(), ins.byRelease.end(), 0);
    std::sort(ins.byRelease.begin(), ins.byRelease.end(), [&](int a, int b) {
        return ins.r[a] != ins.r[b] ? ins.r[a] < ins.r[b] : a < b;
    });
    ins.rankOf.resize(n);
    for (size_t k = 0; k < n; ++k) ins.rankOf[ins.byRelease[k]] = static_cast<int>(k);
    return ins;
}
