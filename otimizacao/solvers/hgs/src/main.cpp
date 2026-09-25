// hgs: Hybrid Genetic Search for P | r_j, s_ij | sum p_j (S_j - r_j) (Copa APA runways).
//
// usage: hgs <instance> [--time s] [--seed k] [--threads k] [--init sol] [--out file|dir]
//            [--xover tw|rx|mix|ruin|none] [--mu k] [--lambda k] [--itNoImp k] [--nbCorr k]
//            [--nbTime k] [--initPop k] [--winMin f] [--winMax f] [--keepBest 0|1]
//            [--verbose k] [--selftest]
#include <sys/stat.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "genetic.hpp"
#include "local_search.hpp"
#include "operators.hpp"

namespace {

bool isDirectory(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

void usage() {
    std::fprintf(stderr,
                 "usage: hgs <instance> [--time s] [--seed k] [--threads k] [--init sol] [--out file|dir]\n"
                 "           [--xover tw|rx|mix|ruin|none] [--mu k] [--lambda k] [--itNoImp k]\n"
                 "           [--nbCorr k] [--nbTime k] [--initPop k] [--winMin f] [--winMax f]\n"
                 "           [--keepBest 0|1] [--verbose k] [--selftest]\n");
}

// Random construction + descent many times; checks incremental cost and state.
int selfTest(const Instance& ins, const Params& par) {
    Rng rng(par.seed);
    LocalSearch ls(ins, par);
    for (int rep = 0; rep < 5; ++rep) {
        Routes rts = randomGreedy(ins, rng);
        // drop some flights and reinsert them to exercise insertMissing
        std::vector<int> missing;
        for (auto& rw : rts)
            for (size_t q = 0; q < rw.size();)
                if (rng.uniform(10) == 0) {
                    missing.push_back(rw[q]);
                    rw.erase(rw.begin() + q);
                } else {
                    ++q;
                }
        ls.load(rts);
        ls.insertMissing(missing);
        if (!ls.consistent()) {
            std::printf("SELFTEST FAIL after insertMissing\n");
            return 1;
        }
        long long before = ls.cost();
        long long after = ls.run(rng);
        if (!ls.consistent() || !validateRoutes(ins, ls.routes()).empty()) {
            std::printf("SELFTEST FAIL after run\n");
            return 1;
        }
        std::printf("selftest rep %d: greedy %lld -> LS %lld (moves %lld) OK\n", rep, before, after,
                    ls.movesApplied());
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }
    Params par;
    std::string instPath = argv[1];
    bool selftest = false, polish = false;
    try {
        for (int i = 2; i < argc; ++i) {
            std::string a = argv[i];
            auto next = [&]() -> std::string {
                if (i + 1 >= argc) throw std::runtime_error("missing value for " + a);
                return argv[++i];
            };
            if (a == "--time") par.timeLimit = std::stod(next());
            else if (a == "--seed") par.seed = std::stoull(next());
            else if (a == "--threads") par.threads = std::max(1, std::stoi(next()));
            else if (a == "--init") par.initFile = next();
            else if (a == "--out") par.outPath = next();
            else if (a == "--xover") par.xover = next();
            else if (a == "--mu") par.mu = std::stoi(next());
            else if (a == "--lambda") par.lambda = std::stoi(next());
            else if (a == "--itNoImp") par.itNoImprove = std::stoi(next());
            else if (a == "--nbCorr") par.nbCorr = std::stoi(next());
            else if (a == "--nbTime") par.nbTime = std::stoi(next());
            else if (a == "--initPop") par.initPop = std::stoi(next());
            else if (a == "--winMin") par.winMin = std::stod(next());
            else if (a == "--winMax") par.winMax = std::stod(next());
            else if (a == "--keepBest") par.keepBest = std::stoi(next()) != 0;
            else if (a == "--eliteEvery") par.eliteEvery = std::stoi(next());
            else if (a == "--archiveSize") par.archiveSize = std::stoi(next());
            else if (a == "--verbose") par.verbose = std::stoi(next());
            else if (a == "--focused") par.focused = std::stoi(next()) != 0;
            else if (a == "--ruinMin") par.ruinMin = std::stod(next());
            else if (a == "--ruinMax") par.ruinMax = std::stod(next());
            else if (a == "--ruinTargeted") par.ruinTargeted = std::stod(next());
            else if (a == "--ruinRouteProb") par.ruinRouteProb = std::stod(next());
            else if (a == "--pTw") par.pTw = std::stod(next());
            else if (a == "--twTargeted") par.twTargeted = std::stod(next());
            else if (a == "--pSortedInsert") par.pSortedInsert = std::stod(next());
            else if (a == "--dist") par.dist = next();
            else if (a == "--ejection") par.ejection = std::stoi(next()) != 0;
            else if (a == "--initFrac") par.initFrac = std::stod(next());
            else if (a == "--nbElite") par.nbElite = std::stoi(next());
            else if (a == "--nbClose") par.nbClose = std::stoi(next());
            else if (a == "--selftest") selftest = true;
            else if (a == "--polish") polish = true;
            else throw std::runtime_error("unknown option " + a);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        usage();
        return 2;
    }

    Instance ins;
    try {
        ins = readInstance(instPath);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 2;
    }
    if (selftest) return selfTest(ins, par);
    if (polish) {
        // Descent + multi-runway tail reassignment on a given solution (diagnostic).
        Rng rng(par.seed);
        LocalSearch ls(ins, par);
        ls.load(readSolution(ins, par.initFile));
        long long c0 = ls.cost();
        ls.run(rng);
        long long c1 = ls.cost();
        int rounds = 0;
        while (ls.cutPass()) {
            ++rounds;
            ls.run(rng);
        }
        std::printf("polish: %lld -> LS %lld -> cut+LS %lld (%d rounds) consistent=%d\n", c0, c1, ls.cost(), rounds,
                    (int)ls.consistent());
        if (!par.outPath.empty()) writeSolution(ins, ls.routes(), ls.cost(), par.outPath);
        return 0;
    }

    std::string out = par.outPath.empty() ? ins.name + "_hgs.txt" : par.outPath;
    if (isDirectory(out)) out += "/" + ins.name + "_hgs_s" + std::to_string(par.seed) + ".txt";

    std::printf("instance %s n=%d m=%d horizon=%d | time %.1fs seed %llu threads %d xover %s mu %d lambda %d\n",
                ins.name.c_str(), ins.n, ins.m, ins.horizon, par.timeLimit, (unsigned long long)par.seed,
                par.threads, par.xover.c_str(), par.mu, par.lambda);
    std::fflush(stdout);

    const Clock::time_point t0 = Clock::now();
    SharedBest shared(ins, out, t0);
    std::vector<GeneticStats> stats(par.threads);
    try {
        std::vector<std::thread> pool;
        for (int k = 0; k < par.threads; ++k)
            pool.emplace_back([&, k]() {
                stats[k] = runGenetic(ins, par, par.seed * 1000003ULL + k, k, par.timeLimit, shared);
            });
        for (auto& th : pool) th.join();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        shared.flush();
        return 1;
    }
    shared.flush();

    long long gens = 0, moves = 0, evals = 0;
    for (const auto& s : stats) {
        gens += s.generations;
        moves += s.lsMoves;
        evals += s.lsEvals;
    }
    const double el = shared.elapsed();
    std::printf("generations %lld (%.1f/s) | LS moves %lld | evals %lld (%.2fM/s) | restarts %lld\n", gens,
                gens / el, moves, evals, evals / el / 1e6, stats[0].restarts);
    if (par.verbose) {
        const char* names[kNumKinds] = {"init", "tw-s1", "tw-s2", "tw-s3", "tw-s4",
                                        "ruin-s1", "ruin-s2", "ruin-s3", "ruin-s4", "rx"};
        std::printf("operator stats (count / child better than both parents / new run best):\n");
        for (int k = 1; k < kNumKinds; ++k) {
            long long cnt = 0, bp = 0, nb = 0;
            for (const auto& s : stats) {
                cnt += s.kindCount[k];
                bp += s.kindBetterParents[k];
                nb += s.kindNewBest[k];
            }
            if (cnt) std::printf("  %-8s %9lld %8lld (%.2f%%) %6lld\n", names[k], cnt, bp, 100.0 * bp / cnt, nb);
        }
    }
    std::printf("FINAL %lld %s\n", shared.cost(), out.c_str());
    return 0;
}
