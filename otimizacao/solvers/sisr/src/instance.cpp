#include "instance.hpp"

#include <algorithm>
#include <climits>
#include <fstream>
#include <stdexcept>

namespace {

std::string baseName(const std::string& path) {
    size_t s = path.find_last_of('/');
    std::string b = (s == std::string::npos) ? path : path.substr(s + 1);
    size_t d = b.find_last_of('.');
    return (d == std::string::npos) ? b : b.substr(0, d);
}

}  // namespace

Instance readInstance(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open instance " + path);
    Instance I;
    I.name = baseName(path);
    if (!(in >> I.n >> I.m) || I.n <= 0 || I.m <= 0)
        throw std::runtime_error("bad header in " + path);
    auto readVec = [&](std::vector<int>& v, size_t cnt) {
        v.resize(cnt);
        for (size_t i = 0; i < cnt; ++i)
            if (!(in >> v[i])) throw std::runtime_error("truncated instance " + path);
    };
    readVec(I.r, I.n);
    readVec(I.c, I.n);
    readVec(I.p, I.n);
    readVec(I.t, static_cast<size_t>(I.n) * I.n);
    I.tminIn.assign(I.n, INT_MAX);
    for (int i = 0; i < I.n; ++i)
        for (int j = 0; j < I.n; ++j)
            if (i != j) I.tminIn[j] = std::min(I.tminIn[j], I.T(i, j));
    for (int j = 0; j < I.n; ++j)
        if (I.tminIn[j] == INT_MAX) I.tminIn[j] = 0;
    return I;
}
