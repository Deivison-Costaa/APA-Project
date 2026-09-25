#include "solution.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

void Solution::init(const Instance& I, const Seqs& seqs) {
    rw.assign(I.m, Runway{});
    rwOf.assign(I.n, -1);
    cost = 0;
    for (int k = 0; k < I.m && k < static_cast<int>(seqs.size()); ++k) {
        rw[k].seq = seqs[k];
        for (int f : seqs[k]) rwOf[f] = k;
        cost += rw[k].rebuild(I);
    }
}

Seqs Solution::seqs() const {
    Seqs out;
    out.reserve(rw.size());
    for (const auto& R : rw) out.push_back(R.seq);
    return out;
}

long long Solution::recomputeCost(const Instance& I) const { return evaluate(I, seqs()); }

long long evaluate(const Instance& I, const Seqs& seqs) {
    long long total = 0;
    for (const auto& q : seqs) {
        int prev = -1;
        long long end = 0;
        for (int f : q) {
            long long s = I.r[f];
            if (prev >= 0) s = std::max<long long>(s, end + I.T(prev, f));
            total += (s - I.r[f]) * I.p[f];
            end = s + I.c[f];
            prev = f;
        }
    }
    return total;
}

Seqs readSolutionFile(const Instance& I, const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open solution " + path);
    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("empty solution " + path);
    Seqs seqs(I.m);
    std::vector<int> seen(I.n, 0);
    for (int k = 0; k < I.m && std::getline(in, line); ++k) {
        std::istringstream ls(line);
        int f1;
        while (ls >> f1) {
            if (f1 < 1 || f1 > I.n) throw std::runtime_error("flight out of range in " + path);
            if (seen[f1 - 1]++) throw std::runtime_error("duplicate flight in " + path);
            seqs[k].push_back(f1 - 1);
        }
    }
    for (int i = 0; i < I.n; ++i)
        if (!seen[i]) throw std::runtime_error("missing flight in " + path);
    return seqs;
}

bool writeSolutionFile(const Instance& I, const Seqs& seqs, long long cost, const std::string& path) {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp);
        if (!out) return false;
        out << cost << "\n";
        for (int k = 0; k < I.m; ++k) {
            const auto& q = seqs[k];
            for (size_t x = 0; x < q.size(); ++x) out << (x ? " " : "") << q[x] + 1;
            out << "\n";
        }
        if (!out) return false;
    }
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}
