#include "instance.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

Instance readInstance(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open instance " + path);
    Instance ins;
    if (!(in >> ins.n >> ins.m) || ins.n <= 0 || ins.m <= 0)
        throw std::runtime_error("bad header in " + path);
    const int n = ins.n;
    auto readVec = [&](std::vector<int>& v) {
        v.resize(n);
        for (int i = 0; i < n; ++i)
            if (!(in >> v[i])) throw std::runtime_error("truncated instance " + path);
    };
    readVec(ins.r);
    readVec(ins.c);
    readVec(ins.p);
    ins.t.resize((size_t)n * n);
    for (size_t k = 0; k < (size_t)n * n; ++k) {
        long long x;
        if (!(in >> x)) throw std::runtime_error("truncated t matrix in " + path);
        if (x < (long long)std::numeric_limits<SetupT>::min() || x > (long long)std::numeric_limits<SetupT>::max())
            throw std::runtime_error("setup time out of range for SetupT; rebuild with -DSETUP_T=int32_t");
        ins.t[k] = (SetupT)x;
    }
    int maxR = *std::max_element(ins.r.begin(), ins.r.end());
    int maxC = *std::max_element(ins.c.begin(), ins.c.end());
    int maxT = *std::max_element(ins.t.begin(), ins.t.end());
    ins.horizon = maxR + maxC + maxT;
    std::string base = path.substr(path.find_last_of('/') == std::string::npos ? 0 : path.find_last_of('/') + 1);
    ins.name = base.substr(0, base.find_last_of('.'));
    return ins;
}

long long evaluateRoutes(const Instance& ins, const Routes& routes, std::vector<int>* S) {
    long long total = 0;
    if (S) S->assign(ins.n, 0);
    for (const auto& rw : routes) {
        int prev = -1, end = 0;
        for (int f : rw) {
            int s = prev < 0 ? ins.r[f] : std::max(ins.r[f], end + ins.T(prev, f));
            total += (long long)ins.p[f] * (s - ins.r[f]);
            if (S) (*S)[f] = s;
            end = s + ins.c[f];
            prev = f;
        }
    }
    return total;
}

std::string validateRoutes(const Instance& ins, const Routes& routes) {
    if ((int)routes.size() != ins.m) return "wrong number of runways";
    std::vector<int> seen(ins.n, 0);
    for (const auto& rw : routes)
        for (int f : rw) {
            if (f < 0 || f >= ins.n) return "flight out of range";
            if (seen[f]++) return "duplicate flight " + std::to_string(f + 1);
        }
    for (int i = 0; i < ins.n; ++i)
        if (!seen[i]) return "missing flight " + std::to_string(i + 1);
    return "";
}

Routes readSolution(const Instance& ins, const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open solution " + path);
    std::string line;
    std::getline(in, line);  // claimed cost (ignored, we recompute)
    Routes routes(ins.m);
    for (int k = 0; k < ins.m && std::getline(in, line); ++k) {
        std::istringstream ls(line);
        int f;
        while (ls >> f) routes[k].push_back(f - 1);
    }
    std::string err = validateRoutes(ins, routes);
    if (!err.empty()) throw std::runtime_error("invalid solution " + path + ": " + err);
    return routes;
}

bool writeSolution(const Instance& ins, const Routes& routes, long long cost, const std::string& path) {
    std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp);
        if (!out) return false;
        out << cost << "\n";
        for (int k = 0; k < ins.m; ++k) {
            const auto& rw = routes[k];
            for (size_t i = 0; i < rw.size(); ++i) out << (i ? " " : "") << rw[i] + 1;
            out << "\n";
        }
        if (!out) return false;
    }
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}
