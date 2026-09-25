#include "ruin.hpp"

#include <algorithm>
#include <limits>
#include <numeric>

namespace {

inline int lowerPos(const Runway& R, int time) {
    return static_cast<int>(std::lower_bound(R.S.begin(), R.S.end(), time) - R.S.begin());
}

}  // namespace

RuinRecreate::RuinRecreate(const Instance& ins, Rng& rng, RuinParams prm)
    : ins_(ins), rng_(rng), prm_(prm), ev_(ins) {
    runwayOrder_.resize(ins.m);
    std::iota(runwayOrder_.begin(), runwayOrder_.end(), 0);
}

int RuinRecreate::pickSeed(const Solution& s) {
    if (rng_.uniform() < prm_.delayedSeed) {
        delayed_.clear();
        for (int f = 0; f < ins_.n; ++f)
            if (s.startOf[f] > ins_.r[f]) delayed_.push_back(f);
        if (!delayed_.empty()) return delayed_[rng_.below(static_cast<int>(delayed_.size()))];
    }
    return rng_.below(ins_.n);
}

void RuinRecreate::perturb(Editor& ed, std::vector<Region>& regions) {
    Solution& s = ed.sol();
    const int seed = pickSeed(s);
    const int tSeed = s.startOf[seed];
    const int seedRunway = s.rwOf[seed];
    for (int i = ins_.m - 1; i > 0; --i) std::swap(runwayOrder_[i], runwayOrder_[rng_.below(i + 1)]);
    std::swap(*std::find(runwayOrder_.begin(), runwayOrder_.end(), seedRunway), runwayOrder_[0]);

    const int nr = rng_.range(1, std::min(ins_.m, prm_.maxRunways));
    removed_.clear();
    for (int i = 0; i < nr; ++i) {
        const int k = runwayOrder_[i];
        const Runway& R = s.rw[k];
        const int len = R.size();
        if (len == 0) continue;
        const int L = std::min(rng_.range(1, prm_.maxString), len);
        const int q = (k == seedRunway) ? s.posOf[seed] : std::min(lowerPos(R, tSeed), len - 1);
        const int lo = std::clamp(q - rng_.below(L), 0, len - L);
        for (int j = lo; j < lo + L; ++j) removed_.push_back(R.seq[j]);
        WindowEdit w;
        w.k = k;
        w.lo = lo;
        w.hi = lo + L;
        w.L = 0;
        ed.applyWindows(&w, 1, tSeed, regions);
    }
    const double u = rng_.uniform();
    if (u < 0.5) {
        std::sort(removed_.begin(), removed_.end(), [&](int a, int b) { return ins_.r[a] < ins_.r[b]; });
    } else if (u < 0.75) {
        for (int i = static_cast<int>(removed_.size()) - 1; i > 0; --i)
            std::swap(removed_[i], removed_[rng_.below(i + 1)]);
    } else {
        std::sort(removed_.begin(), removed_.end(), [&](int a, int b) { return ins_.p[a] > ins_.p[b]; });
    }
    for (int x : removed_) insertBest(ed, x, regions);
}

void RuinRecreate::insertBest(Editor& ed, int x, std::vector<Region>& regions) {
    const Solution& s = ed.sol();
    long long bestD = std::numeric_limits<long long>::max();
    int bestK = -1, bestQ = -1;
    const int w = prm_.insertWindow;
    for (int k = 0; k < ins_.m; ++k) {
        const Runway& R = s.rw[k];
        const int q = lowerPos(R, ins_.r[x]);
        const int hi = std::min(R.size(), q + w);
        for (int qq = std::max(0, q - w); qq <= hi; ++qq) {
            if (prm_.blink > 0 && rng_.uniform() < prm_.blink) continue;
            const long long d = ev_.windowDelta(R, qq, qq, &x, 1);
            if (d < bestD) {
                bestD = d;
                bestK = k;
                bestQ = qq;
            }
        }
    }
    if (bestK < 0) {
        bestK = rng_.below(ins_.m);
        bestQ = lowerPos(s.rw[bestK], ins_.r[x]);
    }
    WindowEdit e;
    e.k = bestK;
    e.lo = bestQ;
    e.hi = bestQ;
    e.L = 1;
    e.list[0] = x;
    ed.applyWindows(&e, 1, ins_.r[x], regions);
}
