#include "search.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

Solution greedyConstruct(const Instance& I) {
    Solution s(I);
    std::vector<int> order(I.n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return I.r[a] < I.r[b]; });
    std::vector<long long> fin(I.m, 0);
    std::vector<int> last(I.m, -1);
    for (int x : order) {
        int bestK = -1;
        long long bestCost = 0, bestGap = 0;
        for (int k = 0; k < I.m; ++k) {
            long long st = I.r[x];
            if (last[k] >= 0) st = std::max<long long>(st, fin[k] + I.T(last[k], x));
            const long long cost = I.p[x] * (st - I.r[x]);
            const long long gap = last[k] >= 0 ? st - fin[k] : (1LL << 40);
            if (bestK < 0 || cost < bestCost || (cost == bestCost && gap < bestGap)) {
                bestK = k;
                bestCost = cost;
                bestGap = gap;
            }
        }
        long long st = I.r[x];
        if (last[bestK] >= 0) st = std::max<long long>(st, fin[bestK] + I.T(last[bestK], x));
        fin[bestK] = st + I.c[x];
        last[bestK] = x;
        s.seq[bestK].push_back(x);
    }
    s.recomputeAll();
    return s;
}

void insertBest(Solution& s, int x, Rng& rng, LocalSearch* ls) {
    const Instance& I = *s.I;
    int bestK = -1, bestQ = 0;
    long long bestD = 0;
    int ties = 0;
    for (int k = 0; k < I.m; ++k) {
        const auto& S = s.st[k];
        const int L = s.len(k);
        const int q0 = static_cast<int>(std::upper_bound(S.begin(), S.end(), I.r[x]) - S.begin());
        const long long old = s.rwCost(k);
        // positions around r_x, extended later for low-penalty flights (cheap to delay)
        const int dmax = std::min(400, 400 / std::max(1, I.p[x]));
        int qEnd = std::min(L, q0 + 1);
        while (qEnd < L && qEnd < q0 + 30 && S[qEnd] <= I.r[x] + dmax) ++qEnd;
        for (int q = std::max(0, q0 - 1); q <= qEnd; ++q) {
            const long long d = s.evalConcat(k, q, &x, 1, k, q) - old;
            if (bestK < 0 || d < bestD) {
                bestK = k;
                bestQ = q;
                bestD = d;
                ties = 1;
            } else if (d == bestD && rng.below(++ties) == 0) {
                bestK = k;
                bestQ = q;
            }
        }
    }
    std::vector<int> ns;
    ns.reserve(s.len(bestK) + 1);
    ns.insert(ns.end(), s.seq[bestK].begin(), s.seq[bestK].begin() + bestQ);
    ns.push_back(x);
    ns.insert(ns.end(), s.seq[bestK].begin() + bestQ, s.seq[bestK].end());
    s.setRunway(bestK, std::move(ns), bestQ);
    if (ls) ls->pushAround(s, bestK, bestQ, 2);
}

void removeFlights(Solution& s, const std::vector<int>& rem) {
    const Instance& I = *s.I;
    std::vector<char> mark(I.n, 0);
    std::vector<int> firstPos(I.m, -1);
    for (int x : rem) {
        mark[x] = 1;
        const int k = s.rwOf[x], q = s.posOf[x];
        if (firstPos[k] < 0 || q < firstPos[k]) firstPos[k] = q;
    }
    for (int k = 0; k < I.m; ++k) {
        if (firstPos[k] < 0) continue;
        std::vector<int> ns;
        ns.reserve(s.len(k));
        for (int y : s.seq[k])
            if (!mark[y]) ns.push_back(y);
        s.setRunway(k, std::move(ns), firstPos[k]);
    }
    for (int x : rem) {
        s.rwOf[x] = -1;
        s.posOf[x] = -1;
    }
}

namespace {
int pickSeed(const Solution& s, Rng& rng, double focus) {
    const Instance& I = *s.I;
    if (focus > 0 && rng.uni() < focus) {
        // reservoir-sample a delayed flight
        int chosen = -1, seen = 0;
        for (int k = 0; k < I.m; ++k)
            for (int q = 0; q < s.len(k); ++q) {
                const int y = s.seq[k][q];
                if (s.st[k][q] > I.r[y] && rng.below(++seen) == 0) chosen = y;
            }
        if (chosen >= 0) return chosen;
    }
    return rng.below(I.n);
}
}  // namespace

std::vector<int> ruinRecreate(Solution& s, LocalSearch& ls, Rng& rng, int kRemove, double focus) {
    const auto& near = ls.neighbors();
    const int f = pickSeed(s, rng, focus);
    std::vector<int> rem{f};
    // take time-neighbors of f, skipping some at random for diversity
    for (int v : near[f]) {
        if (static_cast<int>(rem.size()) >= kRemove) break;
        if (rng.below(3) == 0) continue;
        rem.push_back(v);
    }
    // neighbors on the runways of the removed flights stay; remember them for the LS queue
    std::vector<int> around;
    for (int x : rem) {
        const int k = s.rwOf[x], q = s.posOf[x];
        if (q > 0) around.push_back(s.seq[k][q - 1]);
        if (q + 1 < s.len(k)) around.push_back(s.seq[k][q + 1]);
    }
    removeFlights(s, rem);
    // reinsertion order: random or by release
    if (rng.below(2) == 0) {
        for (int q = static_cast<int>(rem.size()) - 1; q > 0; --q) std::swap(rem[q], rem[rng.below(q + 1)]);
    } else {
        std::sort(rem.begin(), rem.end(), [&](int a, int b) { return s.I->r[a] < s.I->r[b]; });
    }
    for (int x : rem) insertBest(s, x, rng, &ls);
    for (int y : around)
        if (s.rwOf[y] >= 0) ls.push(y);
    for (int x : rem) ls.push(x);
    return rem;
}

Solution runIls(const Instance& I, Solution start, LocalSearch& ls, Rng& rng, const IlsConfig& cfg,
                const Timer& timer, double deadline, const BestCallback& onBest) {
    Solution cur = std::move(start);
    ls.clearQueue();
    ls.pushAll(rng);
    ls.run(cur, rng);
    Solution best = cur;
    if (onBest) onBest(best);
    Solution cand = cur;
    const double t0 = cfg.temp0 > 0 ? cfg.temp0 : std::max(1.0, 0.002 * cur.cost);
    const double t1 = cfg.temp1 > 0 ? cfg.temp1 : std::max(0.2, 0.0001 * cur.cost);
    const double tStart = timer.elapsed();
    long long iter = 0, acc = 0, impr = 0;
    double lastLog = tStart;
    while (true) {
        const double now = timer.elapsed();
        if (now >= deadline) break;
        const double frac = std::min(1.0, (now - tStart) / std::max(1e-9, deadline - tStart));
        const double temp = t0 * std::pow(t1 / t0, frac);
        cand = cur;
        const int k = cfg.kMin + rng.below(cfg.kMax - cfg.kMin + 1);
        ruinRecreate(cand, ls, rng, k, cfg.focus);
        ls.run(cand, rng);
        ++iter;
        const long long d = cand.cost - cur.cost;
        if (d <= 0 || rng.uni() < std::exp(-static_cast<double>(d) / temp)) {
            std::swap(cur, cand);
            ++acc;
            if (cur.cost < best.cost) {
                best = cur;
                ++impr;
                if (onBest) onBest(best);
            }
        }
        if (cfg.verbose && now - lastLog > 5.0) {
            lastLog = now;
            std::printf("[%s] t=%.1f iter=%lld acc=%lld impr=%lld cur=%lld best=%lld T=%.2f evals=%lld\n",
                        cfg.tag.c_str(), now, iter, acc, impr, cur.cost, best.cost, temp, ls.evals);
            std::fflush(stdout);
        }
    }
    if (cfg.verbose)
    {
        std::printf("[%s] ILS done iter=%lld best=%lld (%.1f it/s) evals=%lld moves:", cfg.tag.c_str(), iter,
                    best.cost, iter / std::max(1e-9, timer.elapsed() - tStart), ls.evals);
        static const char* names[] = {"empty", "inRelocate", "inSwap", "relocate", "swap", "2opt*uv",
                                      "2opt*vu", "orOptHead", "segSwap", "orOptTail", "-", "-"};
        for (int q = 0; q < 10; ++q) std::printf(" %s=%lld", names[q], ls.byType[q]);
        std::printf("\n");
    }
    return best;
}
