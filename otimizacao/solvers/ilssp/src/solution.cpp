#include "solution.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unistd.h>

Solution::Solution(const Instance& ins)
    : rw(ins.m), rwOf(ins.n, -1), posOf(ins.n, -1), startOf(ins.n, 0), cost(0) {
    for (auto& R : rw) R.pc.assign(1, 0);
}

int Solution::recompute(const Instance& ins, int k, int from, int structEnd) {
    Runway& R = rw[k];
    const int len = R.size();
    const long long oldCost = R.pc.empty() ? 0 : R.pc.back();
    R.S.resize(len);
    R.pc.resize(len + 1);
    if (from < 0) from = 0;
    if (from == 0) R.pc[0] = 0;
    int last = from - 1;
    int prev = from > 0 ? R.seq[from - 1] : -1;
    int endPrev = from > 0 ? R.S[from - 1] + ins.c[prev] : 0;
    const int* r = ins.r.data();
    const int* c = ins.c.data();
    const int* p = ins.p.data();
    for (int q = from; q < len; ++q) {
        const int f = R.seq[q];
        const int s = prev < 0 ? r[f] : std::max(r[f], endPrev + ins.sep(prev, f));
        if (q < structEnd || s != startOf[f]) last = q;
        R.S[q] = s;
        R.pc[q + 1] = R.pc[q] + static_cast<long long>(p[f]) * (s - r[f]);
        rwOf[f] = k;
        posOf[f] = q;
        startOf[f] = s;
        prev = f;
        endPrev = s + c[f];
    }
    cost += R.pc[len] - oldCost;
    return last;
}

void Solution::recomputeAll(const Instance& ins) {
    cost = 0;
    for (int k = 0; k < static_cast<int>(rw.size()); ++k) {
        rw[k].pc.assign(1, 0);
        rw[k].S.clear();
        recompute(ins, k, 0, rw[k].size());
    }
}

std::vector<std::vector<int>> Solution::sequences() const {
    std::vector<std::vector<int>> out;
    out.reserve(rw.size());
    for (const auto& R : rw) out.push_back(R.seq);
    return out;
}

long long sequenceCost(const Instance& ins, const std::vector<int>& seq) {
    long long total = 0;
    int prev = -1, endPrev = 0;
    for (int f : seq) {
        const int s = prev < 0 ? ins.r[f] : std::max(ins.r[f], endPrev + ins.sep(prev, f));
        total += static_cast<long long>(ins.p[f]) * (s - ins.r[f]);
        prev = f;
        endPrev = s + ins.c[f];
    }
    return total;
}

Solution solutionFromRunways(const Instance& ins, const std::vector<std::vector<int>>& runways) {
    Solution s(ins);
    for (int k = 0; k < ins.m && k < static_cast<int>(runways.size()); ++k) s.rw[k].seq = runways[k];
    s.recomputeAll(ins);
    return s;
}

bool validateSolution(const Instance& ins, const Solution& s, std::string& err) {
    std::vector<int> seen(ins.n, 0);
    long long total = 0;
    if (static_cast<int>(s.rw.size()) != ins.m) {
        err = "wrong runway count";
        return false;
    }
    for (int k = 0; k < ins.m; ++k) {
        const Runway& R = s.rw[k];
        for (int q = 0; q < R.size(); ++q) {
            const int f = R.seq[q];
            if (f < 0 || f >= ins.n) {
                err = "flight out of range";
                return false;
            }
            if (s.rwOf[f] != k || s.posOf[f] != q || s.startOf[f] != R.S[q]) {
                err = "stale position/start cache";
                return false;
            }
            ++seen[f];
        }
        const long long c = sequenceCost(ins, R.seq);
        if (c != R.cost()) {
            err = "runway cost cache mismatch";
            return false;
        }
        total += c;
    }
    for (int f = 0; f < ins.n; ++f) {
        if (seen[f] != 1) {
            err = "flight " + std::to_string(f + 1) + " seen " + std::to_string(seen[f]) + " times";
            return false;
        }
    }
    if (total != s.cost) {
        err = "total cost cache mismatch";
        return false;
    }
    return true;
}

bool readSolutionFile(const std::string& path, const Instance& ins,
                      std::vector<std::vector<int>>& runways, long long& claimed,
                      std::string& err) {
    std::ifstream in(path);
    if (!in) {
        err = "cannot open " + path;
        return false;
    }
    std::string line;
    if (!std::getline(in, line)) {
        err = "empty solution file";
        return false;
    }
    try {
        claimed = std::stoll(line);
    } catch (...) {
        err = "bad cost line";
        return false;
    }
    runways.assign(ins.m, {});
    for (int k = 0; k < ins.m && std::getline(in, line); ++k) {
        std::istringstream ls(line);
        int f;
        while (ls >> f) {
            if (f < 1 || f > ins.n) {
                err = "flight out of range in " + path;
                return false;
            }
            runways[k].push_back(f - 1);
        }
    }
    std::vector<int> seen(ins.n, 0);
    for (const auto& R : runways)
        for (int f : R) ++seen[f];
    for (int f = 0; f < ins.n; ++f) {
        if (seen[f] != 1) {
            err = "flight " + std::to_string(f + 1) + " appears " + std::to_string(seen[f]) + " times";
            return false;
        }
    }
    return true;
}

bool writeSolutionFile(const std::string& path, const Solution& s) {
    const std::string tmp = path + ".tmp" + std::to_string(getpid());
    {
        std::ofstream out(tmp);
        if (!out) return false;
        out << s.cost << "\n";
        for (const auto& R : s.rw) {
            for (size_t q = 0; q < R.seq.size(); ++q) out << (q ? " " : "") << R.seq[q] + 1;
            out << "\n";
        }
        if (!out.good()) return false;
    }
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}
