#include "driver.hpp"

#include <cstdio>

#include "crossover.hpp"
#include "matheur.hpp"
#include "memetic.hpp"
#include <filesystem>
#include <algorithm>
#include "publish.hpp"
#include "search.hpp"

namespace {
void printWindowStats(const char* tag, const WindowStats& w, double t) {
    std::printf(
        "[%s] t=%.1f windows=%lld improved=%lld gain=%lld lbProven=%lld optimal=%lld timeouts=%lld skipped=%lld "
        "avgFree=%.1f avgStates=%.0f avgArcs=%.0f lbTime=%.1fs mipTime=%.1fs (%.3fs/win)\n",
        tag, t, w.calls, w.improved, w.gain, w.provenByLb, w.optimal, w.timeouts, w.skipped,
        w.calls ? double(w.sumFree) / w.calls : 0.0, w.calls ? double(w.sumStates) / w.calls : 0.0,
        w.calls ? double(w.sumArcs) / w.calls : 0.0, w.lbTime, w.mipTime, w.calls ? w.mipTime / w.calls : 0.0);
    std::fflush(stdout);
}

SweepConfig sweepConfig(const Options& o) {
    SweepConfig sc;
    sc.wSize = o.wSize;
    sc.wRunways = o.wRunways;
    sc.verbose = o.verbose;
    sc.mipTimeLimit = o.wtl;
    return sc;
}

IlsConfig ilsConfig(const Options& o, const std::string& tag) {
    IlsConfig cfg;
    cfg.timeLimit = o.timeLimit;
    cfg.kMin = o.kMin;
    cfg.kMax = o.kMax;
    cfg.temp0 = o.temp0;
    cfg.temp1 = o.temp1;
    cfg.focus = o.focus;
    cfg.verbose = o.verbose;
    cfg.tag = tag;
    return cfg;
}
}  // namespace

int runDriver(const Instance& I, const Options& o) {
    Timer timer;
    const std::string name = instanceName(o.instPath);
    Publisher pub(o.outPath, o.poolDir, o.checker, o.instPath, name);
    Rng rng(o.seed);
    Solution start = o.initPath.empty() ? greedyConstruct(I) : readSolution(I, o.initPath);
    std::printf("instance %s n=%d m=%d start cost %lld (%s) mode=%s\n", name.c_str(), I.n, I.m, start.cost,
                o.initPath.empty() ? "greedy" : o.initPath.c_str(), o.mode.c_str());
    pub.offer(start, true);
    LocalSearch ls(I, o.nbK, o.nbW);
    ls.orOpt = o.orOpt != 0;
    WindowOpt wo(I, o.dcap);
    WindowStats wst;
    wst.verbose = o.verbose;
    Solution best = start;
    auto onBest = [&](const Solution& s) { pub.offer(s); };

    if (o.mode == "ils") {
        best = runIls(I, start, ls, rng, ilsConfig(o, name), timer, o.timeLimit, onBest);
    } else if (o.mode == "xtest") {
        std::vector<Solution> sols;
        std::vector<std::string> names;
        for (const auto& e : std::filesystem::directory_iterator(o.poolDir)) {
            const std::string f = e.path().filename().string();
            if (f.rfind(name + "_", 0) != 0) continue;
            try {
                sols.push_back(readSolution(I, e.path().string()));
                names.push_back(f);
            } catch (const std::exception& ex) {
                std::fprintf(stderr, "skip %s: %s\n", f.c_str(), ex.what());
            }
        }
        for (size_t a = 0; a < sols.size(); ++a)
            for (size_t b = 0; b < sols.size(); ++b) {
                if (a == b) continue;
                Solution child(I);
                const long long lim = std::min(sols[a].cost, sols[b].cost);
                const CutResult cr = bestCutChild(sols[a], sols[b], lim + 1000000, child);
                std::printf("%s x %s : best child %lld at T=%d (parents %lld %lld)\n", names[a].c_str(),
                            names[b].c_str(), cr.found ? cr.cost : -1, cr.T, sols[a].cost, sols[b].cost);
                if (cr.found && child.cost < best.cost) best = child;
            }
    } else if (o.mode == "onewin") {
        Solution cur = start;
        std::vector<int> all(I.m);
        for (int k = 0; k < I.m; ++k) all[k] = k;
        const long long d = wo.optimize(cur, o.wt0, o.wt1, all, o.wtl, wst, nullptr);
        std::printf("onewin delta %lld cost %lld\n", d, cur.cost);
        if (cur.cost < best.cost) best = cur;
    } else if (o.mode == "window") {
        Solution cur = start;
        ls.pushAll(rng);
        ls.run(cur, rng);
        std::printf("after LS: %lld\n", cur.cost);
        const SweepConfig sc = sweepConfig(o);
        int sweep = 0;
        while (timer.elapsed() < o.timeLimit) {
            const long long d = windowSweep(cur, wo, ls, rng, sc, wst, timer, o.timeLimit);
            ++sweep;
            std::printf("sweep %d delta %lld cost %lld\n", sweep, d, cur.cost);
            printWindowStats(name.c_str(), wst, timer.elapsed());
            if (cur.cost < best.cost) {
                best = cur;
                onBest(best);
            }
        }
    } else if (o.mode == "hybrid0") {  // ILS phases interleaved with window sweeps
        Solution cur = start;
        const SweepConfig sc = sweepConfig(o);
        IlsConfig cfg = ilsConfig(o, name);
        while (timer.elapsed() < o.timeLimit) {
            const double left = o.timeLimit - timer.elapsed();
            const double ilsEnd = timer.elapsed() + left * (1.0 - o.windowFrac) / 2.0 + 1.0;
            cur = runIls(I, cur, ls, rng, cfg, timer, std::min(o.timeLimit, ilsEnd), onBest);
            if (cur.cost < best.cost) best = cur;
            const long long d = windowSweep(cur, wo, ls, rng, sc, wst, timer, o.timeLimit);
            printWindowStats(name.c_str(), wst, timer.elapsed());
            if (d < 0 && cur.cost < best.cost) {
                best = cur;
                onBest(best);
            }
        }
    } else {  // hybrid (default): epochs of ILS + window sweeps + time-cut crossover archive
        MemeticConfig mc;
        mc.epochs = o.epochs;
        mc.archiveSize = o.archive;
        mc.sweepEpochs = o.windowFrac > 0;
        mc.sweepFrac = o.windowFrac;
        mc.ils = ilsConfig(o, name);
        mc.sweep = sweepConfig(o);
        mc.tag = name;
        mc.verbose = o.verbose;
        MemeticStats mst;
        best = runMemetic(I, start, !o.initPath.empty(), ls, wo, wst, rng, mc, timer, o.timeLimit, onBest, mst);
        std::printf("[%s] crossovers=%lld improvedChildren=%lld newBestFromChildren=%lld\n", name.c_str(),
                    mst.crossovers, mst.childImproved, mst.childBest);
    }
    pub.offer(best, true);
    std::string why;
    if (!best.valid(&why)) {
        std::fprintf(stderr, "ERROR final solution invalid: %s\n", why.c_str());
        return 1;
    }
    if (wst.calls) printWindowStats(name.c_str(), wst, timer.elapsed());
    std::printf("FINAL %s cost %lld time %.1f\n", name.c_str(), best.cost, timer.elapsed());
    return 0;
}
