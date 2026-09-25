#include "crossover.hpp"

#include <algorithm>
#include <cstdio>
#include <numeric>

#include "hungarian.hpp"

namespace {
// Cost of B's runway j suffix from position hi when its head starts at T (sync-stop on B's times).
long long suffixCost(const Solution& B, int j, int hi, long long T) {
    const Instance& I = *B.I;
    const auto& sq = B.seq[j];
    const auto& S = B.st[j];
    const auto& C = B.cum[j];
    const int L = static_cast<int>(sq.size());
    long long cost = 0, fin = 0;
    int prev = -1;
    for (int q = hi; q < L; ++q) {
        const int y = sq[q];
        const long long sy = (q == hi) ? T : std::max<long long>(I.r[y], fin + I.T(prev, y));
        if (sy == S[q]) return cost + C[L] - C[q];
        cost += I.p[y] * (sy - I.r[y]);
        fin = sy + I.c[y];
        prev = y;
    }
    return cost;
}

std::vector<int> startTimes(const Solution& s) {
    std::vector<int> S(s.I->n);
    for (int k = 0; k < s.I->m; ++k)
        for (int q = 0; q < s.len(k); ++q) S[s.seq[k][q]] = s.st[k][q];
    return S;
}
}  // namespace

CutResult bestCutChild(const Solution& A, const Solution& B, long long limit, Solution& child) {
    const Instance& I = *A.I;
    const int n = I.n, m = I.m;
    const auto SA = startTimes(A), SB = startTimes(B);
    int maxT = 0;
    for (int f = 0; f < n; ++f) maxT = std::max({maxT, SA[f], SB[f]});
    std::vector<int> diff(maxT + 3, 0);
    for (int f = 0; f < n; ++f)
        if (SA[f] != SB[f]) {
            const int lo = std::min(SA[f], SB[f]) + 1, hi = std::max(SA[f], SB[f]);
            diff[lo]++;
            diff[hi + 1]--;
        }
    std::vector<int> cand(SA.begin(), SA.end());
    std::sort(cand.begin(), cand.end());
    cand.erase(std::unique(cand.begin(), cand.end()), cand.end());
    std::vector<int> unclean(maxT + 3, 0);
    for (int t = 1; t < maxT + 3; ++t) unclean[t] = unclean[t - 1] + diff[t];
    unclean[0] = diff[0];

    CutResult best;
    std::vector<long long> mat(static_cast<size_t>(m) * m);
    std::vector<int> bestAssign;
    std::vector<int> loA(m), hiB(m);
    for (int T : cand) {
        if (T <= 0 || unclean[T] != 0) continue;
        long long prefix = 0;
        for (int k = 0; k < m; ++k) {
            loA[k] = static_cast<int>(std::lower_bound(A.st[k].begin(), A.st[k].end(), T) - A.st[k].begin());
            prefix += A.cum[k][loA[k]];
            hiB[k] = static_cast<int>(std::lower_bound(B.st[k].begin(), B.st[k].end(), T) - B.st[k].begin());
        }
        if (prefix >= limit) continue;
        for (int k = 0; k < m; ++k) {
            const int prev = loA[k] > 0 ? A.seq[k][loA[k] - 1] : -1;
            const long long fin = prev >= 0 ? A.st[k][loA[k] - 1] + I.c[prev] : 0;
            for (int j = 0; j < m; ++j) {
                long long c = 0;
                if (hiB[j] < B.len(j)) {
                    const int h = B.seq[j][hiB[j]];
                    long long Th = I.r[h];
                    if (prev >= 0) Th = std::max<long long>(Th, fin + I.T(prev, h));
                    c = suffixCost(B, j, hiB[j], Th);
                }
                mat[static_cast<size_t>(k) * m + j] = c;
            }
        }
        const AssignResult asg = hungarian(mat, m);
        const long long total = prefix + asg.cost;
        if (total < limit && (!best.found || total < best.cost)) {
            best.found = true;
            best.cost = total;
            best.T = T;
            bestAssign = asg.colOfRow;
        }
    }
    if (!best.found) return best;
    const int T = best.T;
    child = Solution(I);
    for (int k = 0; k < m; ++k) {
        const int lo = static_cast<int>(std::lower_bound(A.st[k].begin(), A.st[k].end(), T) - A.st[k].begin());
        const int j = bestAssign[k];
        const int hi = static_cast<int>(std::lower_bound(B.st[j].begin(), B.st[j].end(), T) - B.st[j].begin());
        std::vector<int> ns(A.seq[k].begin(), A.seq[k].begin() + lo);
        ns.insert(ns.end(), B.seq[j].begin() + hi, B.seq[j].end());
        child.seq[k] = std::move(ns);
    }
    child.recomputeAll();
    std::string why;
    if (!child.valid(&why) || child.cost != best.cost) {
        std::fprintf(stderr, "crossover: child mismatch (%s) cost %lld expected %lld\n", why.c_str(), child.cost,
                     best.cost);
        best.found = false;
    }
    return best;
}
