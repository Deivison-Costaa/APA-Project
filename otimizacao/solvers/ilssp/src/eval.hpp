// O(1)-in-practice move evaluation with "sync-stop": once a flight of an
// unchanged suffix gets its old start time back, the rest of the runway is
// unchanged and evaluation stops.
#pragma once

#include <cstddef>

#include "solution.hpp"

class Evaluator {
public:
    explicit Evaluator(const Instance& ins)
        : n_(ins.n), r_(ins.r.data()), c_(ins.c.data()), p_(ins.p.data()), t_(ins.t.data()) {}

    // Start time of f placed right after `prev` (which ends at endPrev).
    inline int startAfter(int prev, int endPrev, int f) const {
        if (prev < 0) return r_[f];
        const int s = endPrev + t_[static_cast<size_t>(prev) * n_ + f];
        return s > r_[f] ? s : r_[f];
    }

    // Cost change of the old suffix R[q..] when preceded by `prev` ending at endPrev.
    inline long long tailDelta(const Runway& R, int q, int prev, int endPrev) const {
        long long d = 0;
        const int len = R.size();
        const int* seq = R.seq.data();
        const int* S = R.S.data();
        for (; q < len; ++q) {
            const int f = seq[q];
            const int s = startAfter(prev, endPrev, f);
            if (s == S[q]) break;
            d += static_cast<long long>(p_[f]) * (s - S[q]);
            prev = f;
            endPrev = s + c_[f];
        }
        return d;
    }

    // Schedules `list` after (prev, endPrev); returns its cost and advances prev/endPrev.
    inline long long chain(const int* list, int L, int& prev, int& endPrev) const {
        long long cost = 0;
        for (int i = 0; i < L; ++i) {
            const int f = list[i];
            const int s = startAfter(prev, endPrev, f);
            cost += static_cast<long long>(p_[f]) * (s - r_[f]);
            prev = f;
            endPrev = s + c_[f];
        }
        return cost;
    }

    // Predecessor context (flight, end time) of position q in R.
    inline void context(const Runway& R, int q, int& prev, int& endPrev) const {
        if (q > 0) {
            prev = R.seq[q - 1];
            endPrev = R.S[q - 1] + c_[prev];
        } else {
            prev = -1;
            endPrev = 0;
        }
    }

    // Cost delta of replacing positions [lo, hi) of R by list[0..L).
    inline long long windowDelta(const Runway& R, int lo, int hi, const int* list, int L) const {
        int prev, endPrev;
        context(R, lo, prev, endPrev);
        const long long d = chain(list, L, prev, endPrev) - (R.pc[hi] - R.pc[lo]);
        return d + tailDelta(R, hi, prev, endPrev);
    }

    // 2-opt*: A keeps [0,ca) then B[cb..]; B keeps [0,cb) then A[ca..].
    inline long long twoOptDelta(const Runway& A, int ca, const Runway& B, int cb) const {
        int pa, ea, pb, eb;
        context(A, ca, pa, ea);
        context(B, cb, pb, eb);
        return tailDelta(B, cb, pa, ea) + tailDelta(A, ca, pb, eb);
    }

    int n() const { return n_; }
    int r(int f) const { return r_[f]; }
    int c(int f) const { return c_[f]; }
    int p(int f) const { return p_[f]; }

private:
    int n_;
    const int* r_;
    const int* c_;
    const int* p_;
    const int* t_;
};
