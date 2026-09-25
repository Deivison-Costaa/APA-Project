#include "matheur.hpp"

#include <algorithm>
#include <cstdio>
#include <numeric>

namespace {
std::vector<int> pickRunways(const Solution& s, Rng& rng, int want) {
    const int m = s.I->m;
    std::vector<int> all(m);
    std::iota(all.begin(), all.end(), 0);
    if (want <= 0 || want >= m) return all;
    for (int q = 0; q < want; ++q) std::swap(all[q], all[q + rng.below(m - q)]);
    all.resize(want);
    std::sort(all.begin(), all.end());
    return all;
}
}  // namespace

long long windowSweep(Solution& s, WindowOpt& wo, LocalSearch& ls, Rng& rng, const SweepConfig& cfg,
                      WindowStats& stats, const Timer& timer, double deadline) {
    const Instance& I = *s.I;
    int tmin = 1 << 30, tmax = 0;
    for (int k = 0; k < I.m; ++k)
        if (s.len(k)) {
            tmin = std::min(tmin, s.st[k].front());
            tmax = std::max(tmax, s.st[k].back());
        }
    const int R = (cfg.wRunways <= 0 || cfg.wRunways > I.m) ? I.m : cfg.wRunways;
    const double rate = static_cast<double>(I.n) / std::max(1, tmax - tmin) * R / I.m;
    const int D = std::max(10, static_cast<int>(cfg.wSize / rate));
    const int step = std::max(5, D / 2);
    long long total = 0;
    std::vector<int> touched;
    for (int T0 = tmin - rng.below(step); T0 <= tmax; T0 += step) {
        if (timer.elapsed() >= deadline) break;
        const auto rws = pickRunways(s, rng, R);
        touched.clear();
        const double tl = std::min(cfg.mipTimeLimit, std::max(0.05, deadline - timer.elapsed()));
        const long long d = wo.optimize(s, T0, T0 + D, rws, tl, stats, &touched, cfg.maxFree);
        if (d < 0) {
            total += d;
            for (int u : touched) ls.push(u);
            total += ls.run(s, rng);
            if (cfg.verbose > 1)
                std::printf("  window T0=%d D=%d gain=%lld cost=%lld\n", T0, D, -d, s.cost);
        }
    }
    return total;
}
