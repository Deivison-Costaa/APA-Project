#include "ls.hpp"

#include <algorithm>
#include <cstdlib>
#include <numeric>

namespace {
constexpr int kLateBudget = 400, kLateMaxDelay = 400, kLateMax = 40;

ConcatSpec spec(int P, int a, const int* X, int nx, int Q, int b) {
    ConcatSpec c;
    c.P = P;
    c.a = a;
    c.Q = Q;
    c.b = b;
    c.nx = nx;
    for (int q = 0; q < nx; ++q) c.X[q] = X[q];
    return c;
}
}  // namespace

std::vector<int> buildConcat(const Solution& s, const ConcatSpec& c) {
    std::vector<int> out;
    const int restQ = c.Q >= 0 ? s.len(c.Q) - c.b : 0;
    out.reserve(c.a + c.nx + std::max(0, restQ));
    if (c.P >= 0) out.insert(out.end(), s.seq[c.P].begin(), s.seq[c.P].begin() + c.a);
    out.insert(out.end(), c.X, c.X + c.nx);
    if (c.Q >= 0 && restQ > 0) out.insert(out.end(), s.seq[c.Q].begin() + c.b, s.seq[c.Q].end());
    return out;
}

LocalSearch::LocalSearch(const Instance& inst, int K, int window) : I(inst) {
    const int n = I.n;
    near_.assign(n, {});
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return I.r[a] < I.r[b]; });
    std::vector<int> rank(n);
    for (int q = 0; q < n; ++q) rank[order[q]] = q;
    for (int u = 0; u < n; ++u) {
        std::vector<std::pair<int, int>> cand;
        const int q0 = rank[u];
        for (int q = q0 - 1; q >= 0 && I.r[u] - I.r[order[q]] <= window; --q)
            cand.push_back({I.r[u] - I.r[order[q]], order[q]});
        for (int q = q0 + 1; q < n && I.r[order[q]] - I.r[u] <= window; ++q)
            cand.push_back({I.r[order[q]] - I.r[u], order[q]});
        std::sort(cand.begin(), cand.end());
        if (static_cast<int>(cand.size()) > K) cand.resize(K);
        for (auto& pr : cand) near_[u].push_back(pr.second);
        // Low-penalty flights may be parked far after their release: add later flights up to
        // a delay whose cost stays below kLateBudget (sampled evenly, at most kLateMax extra).
        const int dmax = std::min(kLateMaxDelay, kLateBudget / std::max(1, I.p[u]));
        const int lastNear = cand.empty() ? 0 : cand.back().first;
        std::vector<int> late;
        for (int q = q0 + 1; q < n && I.r[order[q]] - I.r[u] <= dmax; ++q)
            if (I.r[order[q]] - I.r[u] > lastNear) late.push_back(order[q]);
        const int stepL = std::max(1, static_cast<int>(late.size()) / kLateMax);
        for (size_t q = 0; q < late.size(); q += stepL) near_[u].push_back(late[q]);
    }
    inQ_.assign(n, 0);
}

void LocalSearch::push(int u) {
    if (!inQ_[u]) {
        inQ_[u] = 1;
        queue_.push_back(u);
    }
}

void LocalSearch::pushAll(Rng& rng) {
    std::vector<int> order(I.n);
    std::iota(order.begin(), order.end(), 0);
    for (int q = I.n - 1; q > 0; --q) std::swap(order[q], order[rng.below(q + 1)]);
    for (int u : order) push(u);
}

void LocalSearch::pushAround(const Solution& s, int k, int pos, int radius) {
    const int L = s.len(k);
    for (int q = std::max(0, pos - radius); q < std::min(L, pos + radius + 1); ++q) push(s.seq[k][q]);
}

void LocalSearch::clearQueue() {
    for (int u : queue_) inQ_[u] = 0;
    queue_.clear();
}

void LocalSearch::consider(const Solution&, Move& best, long long delta, int nr, const ConcatSpec& s0,
                           const ConcatSpec& s1) {
    if (delta < best.delta) {
        best.delta = delta;
        best.type = curType_;
        best.nr = nr;
        best.spec[0] = s0;
        if (nr > 1) best.spec[1] = s1;
    }
}

void LocalSearch::apply(Solution& s, const Move& mv) {
    std::vector<int> built[2];
    for (int q = 0; q < mv.nr; ++q) built[q] = buildConcat(s, mv.spec[q]);
    for (int q = 0; q < mv.nr; ++q) {
        const ConcatSpec& c = mv.spec[q];
        s.setRunway(c.P, std::move(built[q]), c.a);
    }
    for (int q = 0; q < mv.nr; ++q) {
        const ConcatSpec& c = mv.spec[q];
        const int lo = c.a;
        pushAround(s, c.P, lo, c.nx + 2);
    }
    ++applied;
    ++byType[mv.type];
}

void LocalSearch::sameRunway(const Solution& s, int u, int A, int i, int j, Move& best) {
    // relocate u to just before position j (and after), swap u<->v, within one runway.
    const auto& sq = s.seq[A];
    const long long oldA = s.rwCost(A);
    int X[24];
    auto relocTo = [&](int q) {  // insert u between positions q-1 and q (original indexing)
        if (q == i || q == i + 1 || q < 0 || q > s.len(A)) return;
        int nx = 0;
        long long nc;
        ConcatSpec c;
        if (q < i) {
            X[nx++] = u;
            for (int z = q; z < i; ++z) X[nx++] = sq[z];
            nc = s.evalConcat(A, q, X, nx, A, i + 1);
            c = spec(A, q, X, nx, A, i + 1);
        } else {
            for (int z = i + 1; z < q; ++z) X[nx++] = sq[z];
            X[nx++] = u;
            nc = s.evalConcat(A, i, X, nx, A, q);
            c = spec(A, i, X, nx, A, q);
        }
        ++evals;
        if (nc - oldA < best.delta) consider(s, best, nc - oldA, 1, c, c);
    };
    curType_ = 1;
    relocTo(j);
    relocTo(j + 1);
    // swap
    curType_ = 2;
    const int lo = std::min(i, j), hi = std::max(i, j);
    int nx = 0;
    X[nx++] = sq[hi];
    for (int z = lo + 1; z < hi; ++z) X[nx++] = sq[z];
    X[nx++] = sq[lo];
    const long long nc = s.evalConcat(A, lo, X, nx, A, hi + 1);
    ++evals;
    if (nc - oldA < best.delta) consider(s, best, nc - oldA, 1, spec(A, lo, X, nx, A, hi + 1), ConcatSpec());
}

bool LocalSearch::improveFlight(Solution& s, int u, Move& best) {
    best.delta = 0;
    best.nr = 0;
    const int A = s.rwOf[u], i = s.posOf[u];
    const int LA = s.len(A);
    const auto& sqA = s.seq[A];
    const long long oldA = s.rwCost(A);
    const long long rmA = s.evalConcat(A, i, nullptr, 0, A, i + 1);
    long long rmSeg[4] = {0, 0, 0, 0};    // segment [i, i+L)
    long long rmSegE[4] = {0, 0, 0, 0};   // segment [i-L+1, i]
    for (int L = 2; L <= 3 && orOpt; ++L) {
        if (i + L <= LA) rmSeg[L] = s.evalConcat(A, i, nullptr, 0, A, i + L);
        if (i - L + 1 >= 0) rmSegE[L] = s.evalConcat(A, i - L + 1, nullptr, 0, A, i + 1);
    }
    // empty runways
    curType_ = 0;
    for (int k = 0; k < I.m; ++k)
        if (s.len(k) == 0) {
            const long long d = rmA - oldA;
            if (d < best.delta)
                consider(s, best, d, 2, spec(A, i, nullptr, 0, A, i + 1), spec(k, 0, &u, 1, -1, 0));
            break;
        }
    for (int v : near_[u]) {
        const int B = s.rwOf[v], j = s.posOf[v];
        if (B == A) {
            if (std::abs(i - j) <= sameDist) sameRunway(s, u, A, i, j, best);
            continue;
        }
        const long long oldAB = oldA + s.rwCost(B);
        long long d, nb;
        // relocate u before v / after v
        curType_ = 3;
        nb = s.evalConcat(B, j, &u, 1, B, j);
        d = rmA + nb - oldAB;
        if (d < best.delta)
            consider(s, best, d, 2, spec(A, i, nullptr, 0, A, i + 1), spec(B, j, &u, 1, B, j));
        nb = s.evalConcat(B, j + 1, &u, 1, B, j + 1);
        d = rmA + nb - oldAB;
        if (d < best.delta)
            consider(s, best, d, 2, spec(A, i, nullptr, 0, A, i + 1), spec(B, j + 1, &u, 1, B, j + 1));
        // swap
        curType_ = 4;
        d = s.evalConcat(A, i, &v, 1, A, i + 1) + s.evalConcat(B, j, &u, 1, B, j + 1) - oldAB;
        if (d < best.delta)
            consider(s, best, d, 2, spec(A, i, &v, 1, A, i + 1), spec(B, j, &u, 1, B, j + 1));
        // 2-opt*: arc u->v
        curType_ = 5;
        d = s.evalConcat(A, i + 1, nullptr, 0, B, j) + s.evalConcat(B, j, nullptr, 0, A, i + 1) - oldAB;
        if (d < best.delta)
            consider(s, best, d, 2, spec(A, i + 1, nullptr, 0, B, j), spec(B, j, nullptr, 0, A, i + 1));
        // 2-opt*: arc v->u
        curType_ = 6;
        d = s.evalConcat(B, j + 1, nullptr, 0, A, i) + s.evalConcat(A, i, nullptr, 0, B, j + 1) - oldAB;
        if (d < best.delta)
            consider(s, best, d, 2, spec(B, j + 1, nullptr, 0, A, i), spec(A, i, nullptr, 0, B, j + 1));
        evals += 7;
        // or-opt: segments starting / ending at u, inserted before or after v
        for (int L = 2; L <= 3; ++L) {
            if (i + L <= LA) {
                curType_ = 7;
                const int* seg = &sqA[i];
                for (int q = j; q <= j + 1 && orOpt; ++q) {
                    d = rmSeg[L] + s.evalConcat(B, q, seg, L, B, q) - oldAB;
                    if (d < best.delta)
                        consider(s, best, d, 2, spec(A, i, nullptr, 0, A, i + L), spec(B, q, seg, L, B, q));
                }
                // swap segment [i,i+L) with v
                curType_ = 8;
                d = s.evalConcat(A, i, &v, 1, A, i + L) + s.evalConcat(B, j, seg, L, B, j + 1) - oldAB;
                if (d < best.delta)
                    consider(s, best, d, 2, spec(A, i, &v, 1, A, i + L), spec(B, j, seg, L, B, j + 1));
                evals += 3;
            }
            if (orOpt && i - L + 1 >= 0) {
                curType_ = 9;
                const int* seg = &sqA[i - L + 1];
                for (int q = j; q <= j + 1; ++q) {
                    d = rmSegE[L] + s.evalConcat(B, q, seg, L, B, q) - oldAB;
                    if (d < best.delta)
                        consider(s, best, d, 2, spec(A, i - L + 1, nullptr, 0, A, i + 1),
                                 spec(B, q, seg, L, B, q));
                }
                evals += 2;
            }
        }
    }
    return best.delta < 0;
}

long long LocalSearch::run(Solution& s, Rng& rng) {
    long long total = 0;
    Move mv;
    while (!queue_.empty()) {
        // pop a random element for diversification
        const int idx = rng.below(static_cast<int>(queue_.size()));
        const int u = queue_[idx];
        queue_[idx] = queue_.back();
        queue_.pop_back();
        inQ_[u] = 0;
        if (improveFlight(s, u, mv)) {
            const long long before = s.cost;
            apply(s, mv);
            total += s.cost - before;
            push(u);
        }
    }
    return total;
}
