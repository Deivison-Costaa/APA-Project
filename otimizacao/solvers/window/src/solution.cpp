#include "solution.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

Solution::Solution(const Instance& inst)
    : I(&inst), seq(inst.m), st(inst.m), cum(inst.m, std::vector<long long>(1, 0)),
      rwOf(inst.n, -1), posOf(inst.n, -1), cost(0) {}

void Solution::recompute(int k, int from) {
    const Instance& in = *I;
    const auto& sq = seq[k];
    const int L = static_cast<int>(sq.size());
    auto& S = st[k];
    auto& C = cum[k];
    const long long old = C.empty() ? 0 : C.back();
    S.resize(L);
    C.resize(L + 1);
    if (from < 0) from = 0;
    if (from > L) from = L;
    C[0] = 0;
    int prev = from > 0 ? sq[from - 1] : -1;
    long long fin = from > 0 ? S[from - 1] + in.c[prev] : 0;
    for (int q = from; q < L; ++q) {
        const int y = sq[q];
        int s = in.r[y];
        if (prev >= 0) {
            const long long e = fin + in.T(prev, y);
            if (e > s) s = static_cast<int>(e);
        }
        S[q] = s;
        C[q + 1] = C[q] + static_cast<long long>(in.p[y]) * (s - in.r[y]);
        rwOf[y] = k;
        posOf[y] = q;
        fin = s + in.c[y];
        prev = y;
    }
    cost += C[L] - old;
}

void Solution::recomputeAll() {
    cost = 0;
    for (int k = 0; k < I->m; ++k) {
        cum[k].assign(1, 0);
        recompute(k, 0);
    }
}

void Solution::setRunway(int k, std::vector<int> newSeq, int firstDiff) {
    seq[k] = std::move(newSeq);
    recompute(k, firstDiff);
}

long long Solution::evalConcat(int P, int a, const int* X, int nx, int Q, int b) const {
    const Instance& in = *I;
    long long cost = 0;
    int prev = -1;
    long long fin = 0;
    if (P >= 0 && a > 0) {
        cost = cum[P][a];
        prev = seq[P][a - 1];
        fin = st[P][a - 1] + in.c[prev];
    }
    for (int q = 0; q < nx; ++q) {
        const int x = X[q];
        long long s = in.r[x];
        if (prev >= 0) {
            const long long e = fin + in.T(prev, x);
            if (e > s) s = e;
        }
        cost += in.p[x] * (s - in.r[x]);
        fin = s + in.c[x];
        prev = x;
    }
    if (Q >= 0) {
        const auto& sq = seq[Q];
        const auto& S = st[Q];
        const int L = static_cast<int>(sq.size());
        for (int q = b; q < L; ++q) {
            const int y = sq[q];
            long long s = in.r[y];
            if (prev >= 0) {
                const long long e = fin + in.T(prev, y);
                if (e > s) s = e;
            }
            if (s == S[q]) return cost + cum[Q][L] - cum[Q][q];
            cost += in.p[y] * (s - in.r[y]);
            fin = s + in.c[y];
            prev = y;
        }
    }
    return cost;
}

long long Solution::fullCost() const {
    const Instance& in = *I;
    long long total = 0;
    for (const auto& sq : seq) {
        int prev = -1;
        long long fin = 0;
        for (int y : sq) {
            long long s = in.r[y];
            if (prev >= 0) s = std::max<long long>(s, fin + in.T(prev, y));
            total += in.p[y] * (s - in.r[y]);
            fin = s + in.c[y];
            prev = y;
        }
    }
    return total;
}

bool Solution::valid(std::string* why) const {
    std::vector<int> seen(I->n, 0);
    for (const auto& sq : seq)
        for (int y : sq) {
            if (y < 0 || y >= I->n) {
                if (why) *why = "flight out of range";
                return false;
            }
            seen[y]++;
        }
    for (int i = 0; i < I->n; ++i)
        if (seen[i] != 1) {
            if (why) *why = "flight " + std::to_string(i + 1) + " seen " + std::to_string(seen[i]);
            return false;
        }
    const long long fc = fullCost();
    if (fc != cost) {
        if (why) *why = "cached cost " + std::to_string(cost) + " != " + std::to_string(fc);
        return false;
    }
    return true;
}

Solution readSolution(const Instance& I, const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open solution: " + path);
    std::string line;
    if (!std::getline(in, line)) throw std::runtime_error("empty solution file: " + path);
    Solution s(I);
    for (int k = 0; k < I.m; ++k) {
        if (!std::getline(in, line)) break;
        std::istringstream ls(line);
        int f;
        while (ls >> f) {
            if (f < 1 || f > I.n) throw std::runtime_error("bad flight id in " + path);
            s.seq[k].push_back(f - 1);
        }
    }
    s.recomputeAll();
    std::string why;
    if (!s.valid(&why)) throw std::runtime_error("invalid solution " + path + ": " + why);
    return s;
}

void writeSolution(const Solution& s, const std::string& path) {
    const std::string tmp = path + ".tmp" + std::to_string(getpid());
    {
        std::ofstream out(tmp);
        if (!out) throw std::runtime_error("cannot write " + tmp);
        out << s.fullCost() << "\n";
        for (const auto& sq : s.seq) {
            for (size_t q = 0; q < sq.size(); ++q) out << (q ? " " : "") << sq[q] + 1;
            out << "\n";
        }
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0)
        throw std::runtime_error("rename failed for " + path);
}
