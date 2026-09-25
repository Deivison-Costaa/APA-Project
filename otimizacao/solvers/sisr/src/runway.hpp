// One runway: flight sequence plus start times S and finish times F = S + c.
// All incremental operations use the "sync stop": once a recomputed start time
// equals the stored one (and the predecessor is unchanged), the rest of the
// runway is unchanged, so the recomputation stops.
#pragma once
#include <algorithm>
#include <climits>
#include <vector>

#include "instance.hpp"

constexpr long long kInf = LLONG_MAX / 4;

struct Runway {
    std::vector<int> seq, S, F;
    long long cost = 0;

    int size() const { return static_cast<int>(seq.size()); }

    // Full recomputation from scratch; returns cost.
    long long rebuild(const Instance& I) {
        cost = 0;
        const int L = size();
        S.resize(L);
        F.resize(L);
        for (int x = 0; x < L; ++x) {
            const int f = seq[x];
            const int s = x == 0 ? I.r[f] : std::max(I.r[f], F[x - 1] + I.T(seq[x - 1], f));
            S[x] = s;
            F[x] = s + I.c[f];
            cost += static_cast<long long>(I.p[f]) * (s - I.r[f]);
        }
        return cost;
    }

    // Recompute from index x0 (whose predecessor changed); S[x] hold old values
    // of the same flights, so equality means sync. Updates cost, returns delta.
    long long repair(const Instance& I, int x0) {
        long long d = 0;
        const int L = size();
        const int* sq = seq.data();
        int* s = S.data();
        int* fn = F.data();
        for (int x = x0; x < L; ++x) {
            const int f = sq[x];
            const int ns = x == 0 ? I.r[f] : std::max(I.r[f], fn[x - 1] + I.T(sq[x - 1], f));
            if (ns == s[x]) break;
            d += static_cast<long long>(I.p[f]) * (ns - s[x]);
            s[x] = ns;
            fn[x] = ns + I.c[f];
        }
        cost += d;
        return d;
    }

    // Delta of inserting j before position pos (0..size). Returns kInf when the
    // delta is proven to be >= bound (pruned).
    long long evalInsert(const Instance& I, int j, int pos, long long bound) const {
        const int L = size();
        const int* sq = seq.data();
        const int* s = S.data();
        const int* fn = F.data();
        const int* R = I.r.data();
        const int* P = I.p.data();
        const int* C = I.c.data();
        const int* Tm = I.t.data();
        const size_t n = I.n;
        int sj = R[j];
        if (pos > 0) sj = std::max(sj, fn[pos - 1] + Tm[sq[pos - 1] * n + j]);
        long long d = static_cast<long long>(P[j]) * (sj - R[j]);
        if (d >= bound) return kInf;
        if (pos == L) return d;
        int prev = j;
        int e = sj + C[j];
        int f = sq[pos];
        int ns = std::max(R[f], e + Tm[prev * n + f]);
        int diff = ns - s[pos];
        if (diff == 0) return d;
        d += static_cast<long long>(P[f]) * diff;
        const bool later = diff > 0;
        if (later && d >= bound) return kInf;
        prev = f;
        e = ns + C[f];
        for (int x = pos + 1; x < L; ++x) {
            f = sq[x];
            ns = std::max(R[f], e + Tm[prev * n + f]);
            diff = ns - s[x];
            if (diff == 0) break;
            d += static_cast<long long>(P[f]) * diff;
            if (later && d >= bound) return kInf;
            prev = f;
            e = ns + C[f];
        }
        return d;
    }

    // Insert j before pos; returns cost delta.
    long long insert(const Instance& I, int j, int pos) {
        int sj = I.r[j];
        if (pos > 0) sj = std::max(sj, F[pos - 1] + I.T(seq[pos - 1], j));
        seq.insert(seq.begin() + pos, j);
        S.insert(S.begin() + pos, sj);
        F.insert(F.begin() + pos, sj + I.c[j]);
        const long long d0 = static_cast<long long>(I.p[j]) * (sj - I.r[j]);
        cost += d0;
        return d0 + repair(I, pos + 1);
    }

    // Remove positions [a, b); returns cost delta. Removed flights are appended to out.
    long long eraseRange(const Instance& I, int a, int b) {
        long long d = 0;
        for (int x = a; x < b; ++x) d -= static_cast<long long>(I.p[seq[x]]) * (S[x] - I.r[seq[x]]);
        seq.erase(seq.begin() + a, seq.begin() + b);
        S.erase(S.begin() + a, S.begin() + b);
        F.erase(F.begin() + a, F.begin() + b);
        cost += d;
        return d + repair(I, a);
    }

    int find(int f) const {
        for (int x = 0; x < size(); ++x)
            if (seq[x] == f) return x;
        return -1;
    }

    // First index x with S[x] >= time (size() if none).
    int lowerBoundStart(int time) const {
        return static_cast<int>(std::lower_bound(S.begin(), S.end(), time) - S.begin());
    }
};

// Delta of re-rooting the tail of `T` starting at index ct after a head ending
// with flight `pf` finishing at `pe` (pf < 0 means the tail becomes the runway
// start). Stops as soon as the start times sync, or once the (non-decreasing)
// partial delta reaches `bound` (returns kInf then).
inline long long evalTail(const Instance& I, int pf, int pe, const Runway& Tr, int ct, long long bound) {
    const int L = Tr.size();
    const int* sq = Tr.seq.data();
    const int* s = Tr.S.data();
    const int* R = I.r.data();
    const int* P = I.p.data();
    const int* C = I.c.data();
    const int* Tm = I.t.data();
    const size_t n = I.n;
    long long d = 0;
    int prev = pf, e = pe;
    bool later = true;
    for (int x = ct; x < L; ++x) {
        const int f = sq[x];
        const int ns = prev < 0 ? R[f] : std::max(R[f], e + Tm[prev * n + f]);
        const int diff = ns - s[x];
        if (diff == 0) break;
        if (x == ct) later = diff > 0;
        d += static_cast<long long>(P[f]) * diff;
        if (later && d >= bound) return kInf;
        prev = f;
        e = ns + C[f];
    }
    return d;
}

// Delta of swapping tails: A = A[0..ca) + B[cb..), B = B[0..cb) + A[ca..).
inline long long evalTailSwap(const Instance& I, const Runway& A, int ca, const Runway& B, int cb, long long bound) {
    const int pa = ca > 0 ? A.seq[ca - 1] : -1, ea = ca > 0 ? A.F[ca - 1] : 0;
    const int pb = cb > 0 ? B.seq[cb - 1] : -1, eb = cb > 0 ? B.F[cb - 1] : 0;
    const long long d1 = evalTail(I, pa, ea, B, cb, kInf);
    if (d1 >= kInf) return kInf;
    const long long d2 = evalTail(I, pb, eb, A, ca, bound >= kInf ? kInf : bound - d1);
    if (d2 >= kInf) return kInf;
    return d1 + d2;
}
