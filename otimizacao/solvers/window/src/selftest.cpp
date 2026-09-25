// Self-test: incremental evaluation vs. from-scratch cost; LS deltas vs. real cost.
#include "selftest.hpp"

#include <cstdio>

#include "ls.hpp"
#include "search.hpp"

namespace {
long long seqCost(const Instance& I, const std::vector<int>& sq) {
    long long total = 0, fin = 0;
    int prev = -1;
    for (int y : sq) {
        long long s = I.r[y];
        if (prev >= 0) s = std::max<long long>(s, fin + I.T(prev, y));
        total += I.p[y] * (s - I.r[y]);
        fin = s + I.c[y];
        prev = y;
    }
    return total;
}
}  // namespace

int runSelfTest(const Instance& I, uint64_t seed) {
    Rng rng(seed);
    Solution s = greedyConstruct(I);
    std::string why;
    if (!s.valid(&why)) {
        std::printf("SELFTEST FAIL greedy: %s\n", why.c_str());
        return 1;
    }
    LocalSearch ls(I, 30, 300);
    int fails = 0;
    for (int round = 0; round < 50; ++round) {
        ruinRecreate(s, ls, rng, 10);
        ls.clearQueue();
        if (!s.valid(&why)) {
            std::printf("SELFTEST FAIL after ruin: %s\n", why.c_str());
            return 1;
        }
        for (int trial = 0; trial < 2000; ++trial) {
            const int P = rng.below(I.m), Q = rng.below(I.m);
            const int a = rng.below(s.len(P) + 1), b = rng.below(s.len(Q) + 1);
            int X[3];
            const int nx = rng.below(4);
            for (int q = 0; q < nx; ++q) X[q] = rng.below(I.n);
            ConcatSpec c;
            c.P = P; c.a = a; c.Q = Q; c.b = b; c.nx = nx;
            for (int q = 0; q < nx; ++q) c.X[q] = X[q];
            const long long inc = s.evalConcat(P, a, X, nx, Q, b);
            const long long ref = seqCost(I, buildConcat(s, c));
            if (inc != ref) {
                if (++fails < 5) std::printf("evalConcat mismatch %lld vs %lld\n", inc, ref);
            }
        }
    }
    // LS deltas must match real cost changes
    Solution t = greedyConstruct(I);
    ls.pushAll(rng);
    const long long before = t.cost;
    const long long d = ls.run(t, rng);
    if (t.cost != before + d || !t.valid(&why)) {
        std::printf("SELFTEST FAIL LS: %s\n", why.c_str());
        ++fails;
    }
    std::printf("selftest: greedy=%lld after LS=%lld evals=%lld moves=%lld fails=%d\n", before, t.cost,
                ls.evals, ls.applied, fails);
    return fails ? 1 : 0;
}
