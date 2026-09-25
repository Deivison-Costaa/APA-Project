#include "instance.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

std::string baseName(const std::string& path) {
    size_t s = path.find_last_of('/');
    std::string b = (s == std::string::npos) ? path : path.substr(s + 1);
    size_t d = b.find_last_of('.');
    return d == std::string::npos ? b : b.substr(0, d);
}

}  // namespace

Instance loadInstance(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open instance file: " + path);
    Instance in;
    in.name = baseName(path);
    if (!(f >> in.n >> in.m) || in.n <= 0 || in.m <= 0)
        throw std::runtime_error("bad header (n, m) in " + path);
    const int n = in.n;
    auto readVec = [&](std::vector<int>& v, const char* what) {
        v.resize(n);
        for (int i = 0; i < n; ++i)
            if (!(f >> v[i])) throw std::runtime_error(std::string("truncated array ") + what);
    };
    readVec(in.r, "r");
    readVec(in.c, "c");
    readVec(in.p, "p");
    in.ct.resize(static_cast<size_t>(n) * n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            long long t;
            if (!(f >> t)) throw std::runtime_error("truncated matrix t");
            long long v = in.c[i] + t;
            if (v < 0 || v > 65535) throw std::runtime_error("c_i + t_ij out of uint16 range");
            in.ct[static_cast<size_t>(i) * n + j] = static_cast<uint16_t>(v);
        }
    }
    in.small = *std::max_element(in.ct.begin(), in.ct.end()) <= 255;
    if (in.small) {
        in.ct8.assign(in.ct.begin(), in.ct.end());
        std::vector<uint16_t>().swap(in.ct);
    }
    return in;
}

std::vector<std::vector<int>> readSolutionFile(const Instance& in, const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open solution file: " + path);
    std::string line;
    if (!std::getline(f, line)) throw std::runtime_error("empty solution file: " + path);
    std::vector<std::vector<int>> seqs(in.m);
    std::vector<int> seen(in.n, 0);
    for (int k = 0; k < in.m; ++k) {
        if (!std::getline(f, line)) break;  // missing trailing lines = empty runways
        std::istringstream ss(line);
        int x;
        while (ss >> x) {
            if (x < 1 || x > in.n) throw std::runtime_error("flight out of range in " + path);
            if (seen[x - 1]++) throw std::runtime_error("duplicate flight in " + path);
            seqs[k].push_back(x - 1);
        }
    }
    for (int i = 0; i < in.n; ++i)
        if (!seen[i]) throw std::runtime_error("missing flight in " + path);
    return seqs;
}

void writeSolutionFile(const std::string& path, ll cost, const std::vector<std::vector<int>>& seqs) {
    std::string tmp = path + ".tmpw";
    {
        std::ofstream f(tmp);
        if (!f) throw std::runtime_error("cannot write " + tmp);
        f << cost << "\n";
        for (const auto& s : seqs) {
            for (size_t i = 0; i < s.size(); ++i) f << (i ? " " : "") << s[i] + 1;
            f << "\n";
        }
        if (!f) throw std::runtime_error("write failed: " + tmp);
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) throw std::runtime_error("rename failed: " + path);
}

ll referenceCost(const Instance& in, const std::vector<std::vector<int>>& seqs) {
    ll total = 0;
    for (const auto& s : seqs) {
        ll st = 0;
        int prev = -1;
        for (int f : s) {
            ll start = prev < 0 ? in.r[f] : std::max<ll>(in.r[f], st + in.gap(prev, f));
            total += (start - in.r[f]) * in.p[f];
            st = start;
            prev = f;
        }
    }
    return total;
}
