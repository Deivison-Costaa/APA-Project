#include "localsearch.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace {

inline void setEdit(WindowEdit& w, int k, int lo, int hi, const int* list, int L) {
    w.k = k;
    w.lo = lo;
    w.hi = hi;
    w.L = L;
    for (int i = 0; i < L; ++i) w.list[i] = list[i];
}

inline int lowerPos(const Runway& R, int time) {
    return static_cast<int>(std::lower_bound(R.S.begin(), R.S.end(), time) - R.S.begin());
}

}  // namespace

LocalSearch::LocalSearch(const Instance& ins, Rng& rng, LsParams params)
    : ins_(ins), rng_(rng), prm_(params), ev_(ins), editor_(ins) {
    mask_.assign(ins.n, 0);
    inQueue_.assign(ins.n, 0);
    queue_.reserve(ins.n);
    sortedR_.resize(ins.n);
    for (int i = 0; i < ins.n; ++i) sortedR_[i] = ins.r[ins.byRelease[i]];
    intraBit_ = std::min(ins.m, 63);
    all_ = ins.m >= 63 ? ~uint64_t{0} : ((uint64_t{1} << (ins.m + 1)) - 1);
}

void LocalSearch::attach(Solution* s, Journal* j) {
    sol_ = s;
    editor_.attach(s, j);
    std::fill(mask_.begin(), mask_.end(), 0);
    for (int f : queue_) inQueue_[f] = 0;
    queue_.clear();
}

void LocalSearch::activateAll() {
    for (int f = 0; f < ins_.n; ++f) {
        mask_[f] = all_;
        if (!inQueue_[f]) {
            inQueue_[f] = 1;
            queue_.push_back(f);
        }
    }
}

void LocalSearch::activate(const Region& g) {
    const int lo = static_cast<int>(
        std::lower_bound(sortedR_.begin(), sortedR_.end(), g.tLo - prm_.activationMargin) - sortedR_.begin());
    const int hi = static_cast<int>(
        std::upper_bound(sortedR_.begin(), sortedR_.end(), g.tHi + prm_.activationMargin) - sortedR_.begin());
    const uint64_t bit = bitOf(g.k);
    for (int i = lo; i < hi; ++i) {
        const int f = ins_.byRelease[i];
        mask_[f] |= (sol_->rwOf[f] == g.k) ? all_ : bit;
        if (!inQueue_[f]) {
            inQueue_[f] = 1;
            queue_.push_back(f);
        }
    }
}

int LocalSearch::run() {
    int moves = 0;
    while (!queue_.empty()) {
        const int idx = rng_.below(static_cast<int>(queue_.size()));
        const int x = queue_[idx];
        queue_[idx] = queue_.back();
        queue_.pop_back();
        inQueue_[x] = 0;
        if (mask_[x] && improve(x)) ++moves;
    }
    return moves;
}

bool LocalSearch::improve(int x) {
    const uint64_t mask = mask_[x];
    mask_[x] = 0;
    const int A = sol_->rwOf[x];
    const int a = sol_->posOf[x];
    LsMove best;
    best.delta = 0;
    const long long remDelta = ev_.windowDelta(sol_->rw[A], a, a + 1, nullptr, 0);
    for (int B = 0; B < ins_.m; ++B) {
        if (B == A || !testRunway(mask, B)) continue;
        evalInter(x, A, a, B, remDelta, best);
    }
    if (ins_.m >= 63 || ((mask >> intraBit_) & 1u) || ((mask >> A) & 1u)) evalIntra(x, A, a, best);
    if (best.kind == 0 || best.delta >= 0) return false;
    apply(best, ins_.r[x]);
    return true;
}

void LocalSearch::evalInter(int x, int A, int a, int B, long long remDelta, LsMove& best) {
    const Runway& RA = sol_->rw[A];
    const Runway& RB = sol_->rw[B];
    const int la = RA.size();
    const int lb = RB.size();
    const int q = lowerPos(RB, ins_.r[x]);
    const int w = prm_.posWindow;
    const int qlo = std::max(0, q - w);
    const int qhi = std::min(lb, q + w);

    // relocate x into B before position qq
    for (int qq = qlo; qq <= qhi; ++qq) {
        ++evals_;
        const long long d = remDelta + ev_.windowDelta(RB, qq, qq, &x, 1);
        if (d < best.delta) {
            best.delta = d;
            best.kind = 1;
            best.nEdits = 2;
            setEdit(best.e[0], A, a, a + 1, nullptr, 0);
            setEdit(best.e[1], B, qq, qq, &x, 1);
        }
    }
    // swap x with B[qq]
    for (int qq = qlo; qq <= std::min(lb - 1, qhi); ++qq) {
        ++evals_;
        const int y = RB.seq[qq];
        const long long d = ev_.windowDelta(RA, a, a + 1, &y, 1) + ev_.windowDelta(RB, qq, qq + 1, &x, 1);
        if (d < best.delta) {
            best.delta = d;
            best.kind = 1;
            best.nEdits = 2;
            setEdit(best.e[0], A, a, a + 1, &y, 1);
            setEdit(best.e[1], B, qq, qq + 1, &x, 1);
        }
    }
    // 2-opt*: cut A before x (ca=a) or after x (ca=a+1), cut B near q
    for (int ca = a; ca <= a + 1; ++ca) {
        for (int cb = qlo; cb <= qhi; ++cb) {
            if ((ca == 0 && cb == 0) || (ca == la && cb == lb)) continue;
            ++evals_;
            const long long d = ev_.twoOptDelta(RA, ca, RB, cb);
            if (d < best.delta) {
                best.delta = d;
                best.kind = 2;
                best.A = A;
                best.ca = ca;
                best.B = B;
                best.cb = cb;
            }
        }
    }
    // or-opt: move segment A[a..a+L) into B before qq
    for (int L = 2; L <= 3 && a + L <= la; ++L) {
        const long long rem = ev_.windowDelta(RA, a, a + L, nullptr, 0);
        for (int qq = q; qq <= std::min(lb, q + 1); ++qq) {
            ++evals_;
            const long long d = rem + ev_.windowDelta(RB, qq, qq, &RA.seq[a], L);
            if (d < best.delta) {
                best.delta = d;
                best.kind = 1;
                best.nEdits = 2;
                setEdit(best.e[0], A, a, a + L, nullptr, 0);
                setEdit(best.e[1], B, qq, qq, &RA.seq[a], L);
            }
        }
    }
}

void LocalSearch::evalIntra(int x, int A, int a, LsMove& best) {
    const Runway& RA = sol_->rw[A];
    const int la = RA.size();
    int list[kMaxList];
    // move x forward by d positions
    for (int d = 1; d <= 3 && a + d < la; ++d) {
        for (int i = 0; i < d; ++i) list[i] = RA.seq[a + 1 + i];
        list[d] = x;
        ++evals_;
        const long long v = ev_.windowDelta(RA, a, a + d + 1, list, d + 1);
        if (v < best.delta) {
            best.delta = v;
            best.kind = 1;
            best.nEdits = 1;
            setEdit(best.e[0], A, a, a + d + 1, list, d + 1);
        }
    }
    // move x backward by d positions
    for (int d = 1; d <= 3 && a - d >= 0; ++d) {
        list[0] = x;
        for (int i = 0; i < d; ++i) list[1 + i] = RA.seq[a - d + i];
        ++evals_;
        const long long v = ev_.windowDelta(RA, a - d, a + 1, list, d + 1);
        if (v < best.delta) {
            best.delta = v;
            best.kind = 1;
            best.nEdits = 1;
            setEdit(best.e[0], A, a - d, a + 1, list, d + 1);
        }
    }
}

void LocalSearch::apply(const LsMove& mv, int hint) {
    regions_.clear();
    const long long before = sol_->cost;
    if (mv.kind == 1) editor_.applyWindows(mv.e, mv.nEdits, hint, regions_);
    else editor_.applyTwoOpt(mv.A, mv.ca, mv.B, mv.cb, hint, regions_);
    if (prm_.verify && sol_->cost != before + mv.delta) {
        std::fprintf(stderr, "SELFTEST FAILURE: move kind %d predicted %lld got %lld\n", mv.kind,
                     mv.delta, sol_->cost - before);
        std::abort();
    }
    activate(regions_);
}
