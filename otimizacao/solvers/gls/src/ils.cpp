#include "ils.hpp"

#include "assign.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <thread>

std::vector<std::vector<int>> greedyConstruct(const Instance& in) {
    std::vector<int> order(in.n);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return in.r[a] < in.r[b]; });
    std::vector<std::vector<int>> seqs(in.m);
    std::vector<ll> lastStart(in.m, 0);
    for (int f : order) {
        int bestK = -1;
        ll bestCost = INF_COST, bestReady = 0;
        for (int k = 0; k < in.m; ++k) {
            ll ready = seqs[k].empty() ? -1000000000LL : lastStart[k] + in.gap(seqs[k].back(), f);
            ll s = std::max<ll>(in.r[f], ready);
            ll cost = (s - in.r[f]) * in.p[f];
            // least delay cost; among ties prefer the tightest fit (latest ready time)
            if (cost < bestCost || (cost == bestCost && ready > bestReady)) {
                bestCost = cost;
                bestReady = ready;
                bestK = k;
            }
        }
        ll ready = seqs[bestK].empty() ? in.r[f] : lastStart[bestK] + in.gap(seqs[bestK].back(), f);
        lastStart[bestK] = std::max<ll>(in.r[f], ready);
        seqs[bestK].push_back(f);
    }
    return seqs;
}

namespace {

// caller holds sb.mu
void writeBestLocked(SharedBest& sb, const std::string& path, double now) {
    if (path.empty() || sb.cost >= sb.written) return;
    writeSolutionFile(path, sb.cost, sb.seqs);
    sb.written = sb.cost;
    sb.lastWrite = now;
}

void publish(SharedBest& sb, ll cost, const std::vector<std::vector<int>>& seqs, const std::string& path,
             double now) {
    std::lock_guard<std::mutex> lk(sb.mu);
    if (cost < sb.cost) {
        sb.cost = cost;
        sb.seqs = seqs;
    }
    if (now - sb.lastWrite >= 3.0) writeBestLocked(sb, path, now);
}

void verifyOrDie(const State& st) {
    const ll ref = referenceCost(st.in, st.sequences());
    if (ref != st.total) {
        std::fprintf(stderr, "FATAL: incremental cost %lld != reference %lld\n", st.total, ref);
        std::abort();
    }
}

void chain(int tid, const Instance& in, const Neighbors& nb, const ILSParams& prm,
           const std::vector<std::vector<int>>& init, SharedBest& sb, const Timer& timer) {
    Rng rng(prm.seed * 1000003ULL + static_cast<uint64_t>(tid) * 7919ULL + 17);
    State st(in);
    st.load(init);
    LocalSearch ls(st, nb, prm.ls);
    Perturber pt(st, nb, prm.pert);
    AssignNeighborhood an(st);
    ls.run();
    if (prm.debugCheck) verifyOrDie(st);
    ll cur = st.total;
    ll bestLocal = cur;
    std::vector<std::vector<int>> bestSeqs = st.sequences();
    publish(sb, bestLocal, bestSeqs, prm.outPath, timer.seconds());
    if (prm.verbose && tid == 0)
        std::printf("[t=%.1fs] after initial LS: %lld (evals %lld, moves %lld)\n", timer.seconds(), cur, ls.evals,
                    ls.applied);

    long long iter = 0, acc = 0, lastIter = 0, lastEvals = ls.evals, assignHits = 0;
    double bestTime = timer.seconds();
    long long deltaHist[7] = {0};
    double tPert = 0, tLS = 0, tAssign = 0;
    double lastReport = timer.seconds(), nextSync = prm.syncPeriod;
    const double lnRatio = std::log(prm.Tf / prm.T0);
    const int nCycles = prm.cycle > 0 ? std::max(1, static_cast<int>(std::lround(prm.timeLimit / prm.cycle))) : 1;
    const double cycleLen = prm.timeLimit / nCycles;
    int curCycle = 0;
    while (true) {
        const double now = timer.seconds();
        if (now >= prm.timeLimit || (prm.maxIter >= 0 && iter >= prm.maxIter)) break;
        double frac = prm.maxIter > 0 ? static_cast<double>(iter) / prm.maxIter : now / prm.timeLimit;
        if (nCycles > 1 && prm.maxIter <= 0) {
            const int c = std::min(nCycles - 1, static_cast<int>(now / cycleLen));
            if (c != curCycle) {  // reheat, restart from the best solution of this chain
                curCycle = c;
                st.load(bestSeqs);
                st.clearDirty();
                cur = st.total;
            }
            frac = (now - c * cycleLen) / cycleLen;
        }
        const double T = prm.T0 * std::exp(lnRatio * frac);
        st.begin();
        const double tp0 = timer.seconds();
        pt.ruinRecreate(rng);
        const double tp1 = timer.seconds();
        ls.run();
        const double tp2 = timer.seconds();
        tPert += tp1 - tp0;
        tLS += tp2 - tp1;
        if (prm.assignMode) {
            for (int rep = 0; rep < 3; ++rep) {
                const int T = pt.lastCenter();
                ll g = an.tailAt(T - 40) + an.tailAt(T) + an.tailAt(T + 40);
                g += an.segmentAt(T - 60, T + 60) + an.segmentAt(T - 120, T + 120);
                if (g == 0) break;
                ls.run();
                ++assignHits;
            }
        }
        tAssign += timer.seconds() - tp2;
        const ll nw = st.total;
        ++iter;
        {
            const ll d = nw - cur;
            const int bkt = d < 0 ? 0 : d == 0 ? 1 : d <= 10 ? 2 : d <= 50 ? 3 : d <= 100 ? 4 : d <= 500 ? 5 : 6;
            ++deltaHist[bkt];
        }
        const bool ok = nw <= cur || rng.uniform() < std::exp(-static_cast<double>(nw - cur) / T);
        if (ok) {
            cur = nw;
            ++acc;
            if (nw < bestLocal) {
                bestLocal = nw;
                bestTime = now;
                bestSeqs = st.sequences();
                publish(sb, bestLocal, bestSeqs, prm.outPath, now);
            }
        } else {
            st.rollback();
        }
        if (prm.debugCheck) verifyOrDie(st);
        if (prm.resetFactor > 0 && cur > bestLocal * (1.0 + prm.resetFactor)) {
            st.load(bestSeqs);
            st.clearDirty();
            cur = st.total;
        }
        if (prm.threads > 1 && now >= nextSync) {
            nextSync = now + prm.syncPeriod;
            std::lock_guard<std::mutex> lk(sb.mu);
            // adopt the global best only when this chain is stagnating (keeps diversity)
            if (sb.cost < bestLocal && now - bestTime >= 2 * prm.syncPeriod) {
                bestLocal = sb.cost;
                bestSeqs = sb.seqs;
                st.load(bestSeqs);
                st.clearDirty();
                cur = st.total;
                bestTime = now;
            }
        }
        if (prm.verbose && now - lastReport >= prm.reportEvery) {
            const double dt = now - lastReport;
            std::printf("[t=%.1fs tid=%d] it=%lld it/s=%.0f evals/s=%.2fM acc=%.1f%% T=%.2f cur=%lld best=%lld\n",
                        now, tid, iter, (iter - lastIter) / dt, (ls.evals - lastEvals) / dt / 1e6,
                        100.0 * acc / std::max(1LL, iter), T, cur, bestLocal);
            std::fflush(stdout);
            lastReport = now;
            lastIter = iter;
            lastEvals = ls.evals;
        }
    }
    publish(sb, bestLocal, bestSeqs, prm.outPath, timer.seconds());
    if (prm.verbose) {
        std::printf("[tid=%d] done: iters=%lld (%.0f/s) evals=%lld best=%lld (at %.1fs) assignHits=%lld flights/iter=%.1f "
                    "moves/iter=%.2f\n",
                    tid, iter, iter / std::max(1e-9, timer.seconds()), ls.evals, bestLocal, bestTime, assignHits,
                    static_cast<double>(ls.flightsExamined) / std::max(1LL, iter),
                    static_cast<double>(ls.applied) / std::max(1LL, iter));
        std::printf("   LS: flights examined %lld, pruned candidates %lld\n", ls.flightsExamined, ls.pruned);
        std::printf("   time: perturb %.1fs  LS %.1fs  assign+LS %.1fs\n", tPert, tLS, tAssign);
        std::printf("   delta hist: <0 %lld | 0 %lld | 1-10 %lld | 11-50 %lld | 51-100 %lld | 101-500 %lld | >500 %lld\n",
                    deltaHist[0], deltaHist[1], deltaHist[2], deltaHist[3], deltaHist[4], deltaHist[5], deltaHist[6]);
        static const char* names[] = {"relocP", "crossP", "2optP", "relocS", "crossS", "2optS", "intra", "starts"};
        for (int t = 0; t < LocalSearch::NTYPES; ++t)
            std::printf("   %-7s evals=%6.2f%% applied=%lld\n", names[t], 100.0 * ls.evalsBy[t] / std::max(1LL, ls.evals),
                        ls.appliedBy[t]);
    }
}

}  // namespace

void runILS(const Instance& in, const Neighbors& nb, const ILSParams& prm,
            const std::vector<std::vector<int>>& init, SharedBest& best) {
    Timer timer;
    const int k = std::max(1, prm.threads);
    std::vector<std::thread> th;
    for (int t = 1; t < k; ++t)
        th.emplace_back(chain, t, std::cref(in), std::cref(nb), std::cref(prm), std::cref(init), std::ref(best),
                        std::cref(timer));
    chain(0, in, nb, prm, init, best, timer);
    for (auto& x : th) x.join();
    std::lock_guard<std::mutex> lk(best.mu);
    writeBestLocked(best, prm.outPath, timer.seconds());
}
