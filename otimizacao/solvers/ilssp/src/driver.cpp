#include "driver.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <thread>

namespace fs = std::filesystem;

namespace {

double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

constexpr size_t kMaxPool = 400000;    // columns kept in memory
constexpr long long kKeepRounds = 4;   // rounds of history kept when trimming

std::vector<int> columnsOf(const Solution& s, ColumnPool& pool, long long stamp) {
    std::vector<int> ids;
    for (const Runway& R : s.rw) {
        if (R.seq.empty()) continue;
        const int id = pool.add(R.seq, R.cost(), 2, stamp);
        if (id >= 0) ids.push_back(id);
    }
    return ids;
}

std::vector<int> selectColumns(const ColumnPool& pool, const std::vector<int>& mustHave, int maxCols) {
    std::vector<int> ids(pool.size());
    for (size_t i = 0; i < pool.size(); ++i) ids[i] = static_cast<int>(i);
    if (static_cast<int>(ids.size()) <= maxCols) return ids;
    std::vector<char> forced(pool.size(), 0);
    for (int id : mustHave) forced[id] = 1;
    for (size_t i = 0; i < pool.size(); ++i)
        if (pool.col(i).source == 1) forced[i] = 1;
    std::sort(ids.begin(), ids.end(), [&](int a, int b) {
        if (forced[a] != forced[b]) return forced[a] > forced[b];
        if (pool.col(a).lastSeen != pool.col(b).lastSeen) return pool.col(a).lastSeen > pool.col(b).lastSeen;
        return pool.col(a).hits > pool.col(b).hits;
    });
    ids.resize(maxCols);
    return ids;
}

Solution solutionFromColumns(const Instance& ins, const ColumnPool& pool, const std::vector<int>& chosen) {
    std::vector<std::vector<int>> runways;
    for (int id : chosen) runways.push_back(pool.col(id).seq);
    return solutionFromRunways(ins, runways);
}

}  // namespace

struct Driver::Worker {
    Worker(const Instance& ins, const Options& o, const IlsParams& ip, uint64_t seed)
        : ils(ins, seed, ip, o.ls, o.ruin) {}
    Ils ils;
    Solution cur, best;
    ColumnPool pool;
    IlsStats st;
};

Driver::Driver(const Instance& ins, const Options& opt) : ins_(ins), opt_(opt) {
    IlsParams ip = opt.ils;
    ip.stop = &stop_;
    for (int w = 0; w < opt.threads; ++w)
        workers_.push_back(std::make_unique<Worker>(ins, opt, ip, opt.seed * 1000003ULL + w));
}

Driver::~Driver() = default;

int Driver::ingest(Solution& best) {
    const std::string& dir = opt_.poolDir;
    if (dir.empty() || !fs::is_directory(dir)) return 0;
    int files = 0;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        const std::string name = entry.path().filename().string();
        if (name.empty() || name[0] == '.' || entry.path().extension() != ".txt") continue;
        if (!ingested_.insert(name).second) continue;
        std::vector<std::vector<int>> runways;
        long long claimed = 0;
        std::string err;
        if (!readSolutionFile(entry.path().string(), ins_, runways, claimed, err)) continue;
        const Solution s = solutionFromRunways(ins_, runways);
        for (const Runway& R : s.rw)
            if (!R.seq.empty()) pool_.add(R.seq, R.cost(), 1, 0);
        ++files;
        if (s.cost < best.cost) {
            best = s;
            std::printf("  ingested %s -> new best %lld\n", name.c_str(), s.cost);
        }
    }
    return files;
}

void Driver::mergePools(long long round, const Solution& best) {
    for (auto& w : workers_) {
        for (size_t i = 0; i < w->pool.size(); ++i) {
            const Column& c = w->pool.col(i);
            pool_.add(c.seq, c.cost, c.source, c.lastSeen);
        }
        w->pool.clear();
    }
    if (pool_.size() <= kMaxPool) return;
    std::vector<char> keep(pool_.size(), 0);
    for (size_t i = 0; i < pool_.size(); ++i)
        keep[i] = pool_.col(i).source == 1 || pool_.col(i).lastSeen >= round - kKeepRounds;
    for (const Runway& R : best.rw) {
        const int id = pool_.find(R.seq);
        if (id >= 0) keep[id] = 1;
    }
    pool_.retain(keep);
}

bool Driver::runSp(Solution& best, double deadline, long long round, const std::string& tag) {
    const std::vector<int> warm = columnsOf(best, pool_, round);
    const std::vector<int> ids = selectColumns(pool_, warm, opt_.spMaxCols);
    const double lim = std::min(opt_.spTime, deadline - nowSec() - 0.1);
    if (lim < 0.5) return false;
    const SpResult sp = solveSetPartitioning(ins_, pool_, ids, warm, lim, opt_.spVerbose);
    ++sp_.solves;
    sp_.totalTime += sp.seconds;
    sp_.maxTime = std::max(sp_.maxTime, sp.seconds);
    sp_.maxCols = std::max(sp_.maxCols, ids.size());
    bool improved = false;
    std::string err;
    if (sp.feasible && sp.cost < best.cost) {
        Solution s = solutionFromColumns(ins_, pool_, sp.chosen);
        if (s.cost == sp.cost && validateSolution(ins_, s, err)) {
            best = s;
            improved = true;
            ++sp_.improved;
        }
    }
    std::printf("%s SP cols %zu/%zu -> %s %lld%s (bound %.0f, %.1fs)%s\n", tag.c_str(), ids.size(),
                pool_.size(), sp.feasible ? "feasible" : "no-solution", sp.feasible ? sp.cost : -1LL,
                sp.optimal ? " optimal" : "", sp.lpBound, sp.seconds, improved ? " IMPROVED" : "");
    return improved;
}

Solution Driver::solve(Solution best, double tStart, double deadline, const std::string& outFile) {
    ingest(best);
    writeSolutionFile(outFile, best);
    std::mutex mtx;
    long long printedBest = best.cost;
    long long bestSeen = best.cost;
    double lastPrint = -1.0, bestTime = 0.0;  // bestTime: when the best cost was first reached
    auto reached = [&](long long cost) { return opt_.target >= 0 && cost <= opt_.target; };
    if (reached(best.cost)) stop_ = true;
    auto onBest = [&](const Solution& s) {
        std::lock_guard<std::mutex> lock(mtx);
        const double t = nowSec() - tStart;
        if (s.cost < bestSeen) {
            bestSeen = s.cost;
            bestTime = t;
            if (reached(s.cost)) stop_ = true;
        }
        if (s.cost < printedBest && !opt_.quiet && t - lastPrint >= 0.5) {
            std::printf("  t=%7.1f best %lld\n", t, s.cost);
            lastPrint = t;
            printedBest = s.cost;
        }
    };
    long long round = 0;
    while (nowSec() < deadline - 0.05 && !stop_) {
        ++round;
        const double remaining = deadline - nowSec();
        const bool doSp = opt_.useSp && remaining > opt_.roundTime * 0.5;
        const double spReserve = doSp ? std::min(opt_.spTime + 0.5, remaining * 0.3) : 0.0;
        const double ilsTime = std::max(0.01, doSp ? std::min(opt_.roundTime, remaining - spReserve) : remaining);
        const long long before = best.cost;
        std::vector<std::thread> threads;
        for (auto& wp : workers_) {
            Worker* w = wp.get();
            w->cur = best;
            w->best = best;
            ColumnPool* pool = opt_.useSp ? &w->pool : nullptr;
            threads.emplace_back([w, ilsTime, pool, round, &onBest]() {
                w->ils.run(w->cur, w->best, ilsTime, pool, round, w->st, onBest);
            });
        }
        for (auto& t : threads) t.join();
        for (auto& w : workers_)
            if (w->best.cost < best.cost) best = w->best;
        const long long ilsBest = best.cost;
        char tag[96];
        std::snprintf(tag, sizeof tag, "round %lld [t=%.0f]: ils %lld -> %lld |", round, nowSec() - tStart,
                      before, ilsBest);
        if (doSp && nowSec() < deadline - 0.5) {
            ingest(best);
            mergePools(round, best);
            runSp(best, deadline, round, tag);
        } else {
            std::printf("%s\n", tag);
        }
        writeSolutionFile(outFile, best);
    }
    long long iters = 0, acc = 0, evals = 0;
    for (auto& w : workers_) {
        iters += w->st.iterations;
        acc += w->st.accepted;
        evals += w->ils.evaluations();
    }
    const double el = nowSec() - tStart;
    std::printf("final %lld | best at %.2fs%s | time %.1fs | threads %d | iters %lld (%.0f/s) acc %lld | evals %lld (%.2fM/s) | "
                "pool %zu | SP solves %d improved %d avg %.2fs max %.2fs maxcols %zu\n",
                best.cost, bestTime, reached(best.cost) ? " (target)" : "", el, opt_.threads, iters, iters / el, acc, evals, evals / el / 1e6, pool_.size(),
                sp_.solves, sp_.improved, sp_.solves ? sp_.totalTime / sp_.solves : 0.0, sp_.maxTime,
                sp_.maxCols);
    return best;
}
