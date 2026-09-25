#include "instance.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
std::vector<long long> readAllInts(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open instance file: " + path);
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string data = ss.str();
    std::vector<long long> out;
    out.reserve(data.size() / 3);
    const char* s = data.c_str();
    while (*s) {
        while (*s && !(*s == '-' || (*s >= '0' && *s <= '9'))) ++s;
        if (!*s) break;
        bool neg = false;
        if (*s == '-') { neg = true; ++s; }
        long long v = 0;
        while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); ++s; }
        out.push_back(neg ? -v : v);
    }
    return out;
}
}  // namespace

Instance Instance::read(const std::string& path) {
    const auto v = readAllInts(path);
    if (v.size() < 2) throw std::runtime_error("instance too short: " + path);
    Instance I;
    I.n = static_cast<int>(v[0]);
    I.m = static_cast<int>(v[1]);
    const size_t n = I.n;
    if (I.n <= 0 || I.m <= 0) throw std::runtime_error("bad n/m in " + path);
    if (v.size() != 2 + 3 * n + n * n)
        throw std::runtime_error("bad instance size in " + path);
    I.r.assign(v.begin() + 2, v.begin() + 2 + n);
    I.c.assign(v.begin() + 2 + n, v.begin() + 2 + 2 * n);
    I.p.assign(v.begin() + 2 + 2 * n, v.begin() + 2 + 3 * n);
    I.t.assign(v.begin() + 2 + 3 * n, v.end());
    bool small = true;
    for (int x : I.t)
        if (x < 0 || x > 255) { small = false; break; }
    if (small) I.t8.assign(I.t.begin(), I.t.end());
    return I;
}
