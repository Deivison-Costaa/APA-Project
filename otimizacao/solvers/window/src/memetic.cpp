#include "memetic.hpp"

#include <algorithm>
#include <cstdio>
#include <deque>

#include "crossover.hpp"

namespace {
uint64_t mix(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// Runway-order independent signature of the arc set.
uint64_t signature(const Solution& s) {
    const uint64_t n = s.I->n;
    uint64_t h = 0;
    for (const auto& sq : s.seq) {
        uint64_t prev = n;  // runway start
        for (int y : sq) {
            h += mix(prev * (n + 1) + static_cast<uint64_t>(y));
            prev = static_cast<uint64_t>(y);
        }
    }
    return h;
}

struct Archive {
    int cap;
    std::vector<Solution> sols;
    std::vector<uint64_t> sig;

    bool insert(const Solution& s) {
        const uint64_t g = signature(s);
        if (std::find(sig.begin(), sig.end(), g) != sig.end()) return false;
        if (static_cast<int>(sols.size()) >= cap) {
            const auto worst = std::max_element(sols.begin(), sols.end(),
                                                [](const Solution& a, const Solution& b) { return a.cost < b.cost; });
            if (worst->cost <= s.cost) return false;
            const size_t w = static_cast<size_t>(worst - sols.begin());
            sols[w] = s;
            sig[w] = g;
            return true;
        }
        sols.push_back(s);
        sig.push_back(g);
        return true;
    }
};

void polishAround(Solution& s, LocalSearch& ls, Rng& rng, int T, int radius) {
    ls.clearQueue();
    for (int k = 0; k < s.I->m; ++k)
        for (int q = 0; q < s.len(k); ++q)
            if (std::abs(s.st[k][q] - T) <= radius) ls.push(s.seq[k][q]);
    ls.run(s, rng);
}

int windowSpan(const Solution& s, int wSize) {
    int tmin = 1 << 30, tmax = 0;
    for (int k = 0; k < s.I->m; ++k)
        if (s.len(k)) {
            tmin = std::min(tmin, s.st[k].front());
            tmax = std::max(tmax, s.st[k].back());
        }
    return std::max(10, static_cast<int>(static_cast<double>(wSize) * (tmax - tmin) / s.I->n));
}
}  // namespace

Solution runMemetic(const Instance& I, Solution first, bool firstIsInit, LocalSearch& ls, WindowOpt& wo,
                    WindowStats& wst, Rng& rng, const MemeticConfig& cfg, const Timer& timer, double deadline,
                    const BestCallback& onBest, MemeticStats& mst) {
    Archive ar{cfg.archiveSize, {}, {}};
    Solution best = first;
    std::vector<int> allRw(I.m);
    for (int k = 0; k < I.m; ++k) allRw[k] = k;
    auto consider = [&](const Solution& s) {
        if (s.cost < best.cost) {
            best = s;
            if (onBest) onBest(best);
            return true;
        }
        return false;
    };
    // Crossover of x against the archive (both directions); improved children are recombined too.
    auto recombine = [&](const Solution& x) {
        std::deque<Solution> todo{x};
        while (!todo.empty() && timer.elapsed() < deadline) {
            const Solution c = std::move(todo.front());
            todo.pop_front();
            const uint64_t cs = signature(c);
            const std::vector<Solution> members = ar.sols;
            for (const Solution& a : members) {
                if (signature(a) == cs) continue;
                for (int dir = 0; dir < 2; ++dir) {
                    const Solution& P = dir == 0 ? c : a;
                    const Solution& Q = dir == 0 ? a : c;
                    Solution child(I);
                    ++mst.crossovers;
                    const CutResult cr = bestCutChild(P, Q, std::min(P.cost, Q.cost), child);
                    if (!cr.found) continue;
                    polishAround(child, ls, rng, cr.T, 200);
                    if (cfg.cutWindow) {
                        const int D = windowSpan(child, cfg.sweep.wSize);
                        std::vector<int> touched;
                        if (wo.optimize(child, cr.T - D / 2, cr.T + D / 2, allRw, cfg.sweep.mipTimeLimit, wst,
                                        &touched, cfg.sweep.maxFree) < 0) {
                            for (int u : touched) ls.push(u);
                            ls.run(child, rng);
                        }
                    }
                    ++mst.childImproved;
                    if (cfg.verbose)
                        std::printf("[%s] t=%.1f crossover %lld x %lld at T=%d -> %lld (polished %lld)\n",
                                    cfg.tag.c_str(), timer.elapsed(), P.cost, Q.cost, cr.T, cr.cost, child.cost);
                    if (consider(child)) ++mst.childBest;
                    if (ar.insert(child)) todo.push_back(child);
                }
            }
        }
    };

    for (int e = 0; timer.elapsed() < deadline - 1.0; ++e) {
        const double now = timer.elapsed();
        const double left = deadline - now;
        const int epochsLeft = std::max(1, cfg.epochs - e);
        const double epochEnd = now + left / epochsLeft;
        const double sweepBudget = cfg.sweepEpochs ? (epochEnd - now) * cfg.sweepFrac : 0.0;
        Solution start = (e == 0) ? first : greedyConstruct(I);
        IlsConfig ic = cfg.ils;
        if (e == 0 && firstIsInit && ic.temp0 <= 0) {
            ic.temp0 = std::max(1.0, 0.0005 * start.cost);  // gentler anneal from an elite start
        }
        Solution sol = runIls(I, start, ls, rng, ic, timer, epochEnd - sweepBudget, onBest);
        consider(sol);
        if (cfg.sweepEpochs && timer.elapsed() < epochEnd) {
            const long long d = windowSweep(sol, wo, ls, rng, cfg.sweep, wst, timer, epochEnd);
            if (cfg.verbose)
                std::printf("[%s] t=%.1f epoch %d sweep delta %lld -> %lld\n", cfg.tag.c_str(), timer.elapsed(), e,
                            d, sol.cost);
            consider(sol);
        }
        if (cfg.verbose)
            std::printf("[%s] t=%.1f epoch %d result %lld best %lld archive %zu\n", cfg.tag.c_str(),
                        timer.elapsed(), e, sol.cost, best.cost, ar.sols.size() + 1);
        std::fflush(stdout);
        recombine(sol);
        ar.insert(sol);
    }
    return best;
}
