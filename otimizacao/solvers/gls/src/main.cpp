// gls: granular local search + ILS (ruin & recreate, SA acceptance) for
// P | r_j, s_ij | sum p_j (S_j - r_j)   (Copa APA runway scheduling).
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include "assign.hpp"
#include "ils.hpp"
#include "recombine.hpp"

namespace {

void usage() {
    std::printf(
        "usage: gls <instance> [--time s] [--seed k] [--threads k] [--init sol] [--out file|dir]\n"
        "  tuning: --K n --idle w --seg n --cross n --first 0|1 --T0 x --Tf x --kmin n --kmax n\n"
        "          --pwin x --maxrw n --inswin n --blink x --pdelay x --reset x --sync s --cycle s\n"
        "  --check     verify incremental cost against reference after every iteration (slow)\n"
        "  --selftest  run internal consistency tests and exit\n");
}

bool isDir(const std::string& p) {
    struct stat sb;
    return stat(p.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}

int selfTest(const std::string& instPath, const ILSParams& base, const Neighbors& nb, const Instance& in) {
    // 1) the statement example: "1 2 5 / 3 4 6" costs 2800 (only if this is the example instance)
    if (in.n == 6 && in.m == 2) {
        std::vector<std::vector<int>> s = {{0, 1, 4}, {2, 3, 5}};
        const ll c = referenceCost(in, s);
        State st(in);
        st.load(s);
        std::printf("example: reference=%lld incremental=%lld (expected 2800)\n", c, st.total);
        if (c != 2800 || st.total != 2800) return 1;
    }
    // 2) short ILS with verification after every iteration, 3 seeds
    for (int seed = 1; seed <= 3; ++seed) {
        ILSParams prm = base;
        prm.timeLimit = 2.0;
        prm.seed = seed;
        prm.threads = 1;
        prm.debugCheck = true;
        prm.verbose = false;
        prm.outPath.clear();
        SharedBest sb;
        runILS(in, nb, prm, greedyConstruct(in), sb);
        const ll ref = referenceCost(in, sb.seqs);
        std::printf("selftest %s seed %d: best=%lld reference=%lld %s\n", instPath.c_str(), seed, sb.cost, ref,
                    ref == sb.cost ? "OK" : "MISMATCH");
        if (ref != sb.cost) return 1;
    }
    return 0;
}

// Diagnostic: local search, then sweeps of the assignment neighborhoods until no improvement.
int sweepTest(const Instance& in, const Neighbors& nb, const ILSParams& prm,
              const std::vector<std::vector<int>>& init) {
    Timer tm;
    State st(in);
    st.load(init);
    LocalSearch ls(st, nb, prm.ls);
    ls.run();
    std::printf("after LS: %lld\n", st.total);
    AssignNeighborhood an(st);
    for (int round = 0; round < 20; ++round) {
        ll gainT = 0, gainS = 0;
        const int horizon = in.r[nb.byRelease.back()] + 200;
        for (int T = 0; T <= horizon; T += 5) {
            gainT += an.tailAt(T);
            for (int W : {50, 100, 200, 400}) gainS += an.segmentAt(T, T + W);
        }
        ls.run();
        std::printf("round %d: tail gain %lld seg gain %lld -> after LS %lld (%.2fs, ref %lld)\n", round, gainT,
                    gainS, st.total, tm.seconds(), referenceCost(in, st.sequences()));
        if (gainT == 0 && gainS == 0) break;
    }
    return 0;
}

// Diagnostic: window-import crossover of the initial solution with donor solutions.
int recombineTest(const Instance& in, const Neighbors& nb, const ILSParams& prm,
                  const std::vector<std::vector<int>>& init, const std::vector<std::string>& donors) {
    State st(in);
    st.load(init);
    LocalSearch ls(st, nb, prm.ls);
    ls.run();
    AssignNeighborhood an(st);
    WindowImport wi(st);
    ll cur = st.total;
    std::printf("start %lld\n", cur);
    const int horizon = in.r[nb.byRelease.back()] + 1;
    long long tries = 0;
    for (const auto& dpath : donors) {
        const auto D = readSolutionFile(in, dpath);
        for (int W : {60, 120, 200, 300, 450}) {
            for (int T1 = -W / 2; T1 < horizon; T1 += std::max(10, W / 6)) {
                st.begin();
                if (!wi.apply(D, T1, T1 + W)) { st.rollback(); continue; }
                ++tries;
                ls.run();
                for (int rep = 0; rep < 3; ++rep) {
                    ll g = 0;
                    for (int T = T1 - 40; T <= T1 + W + 40; T += 20) g += an.tailAt(T) + an.segmentAt(T, T + 100);
                    if (g == 0) break;
                    ls.run();
                }
                if (st.total < cur) {
                    std::printf("  improved %lld -> %lld with %s window [%d,%d)\n", cur, st.total, dpath.c_str(), T1,
                                T1 + W);
                    cur = st.total;
                } else {
                    st.rollback();
                }
            }
        }
    }
    std::printf("final %lld (tries %lld, ref %lld)\n", cur, tries, referenceCost(in, st.sequences()));
    if (!prm.outPath.empty()) writeSolutionFile(prm.outPath, cur, st.sequences());
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }
    try {
        std::string instPath = argv[1], initPath, outArg;
        ILSParams prm;
        int K = 30;
        double idleW = 1.0;
        bool selftest = false, sweep = false;
        std::vector<std::string> donors;
        for (int a = 2; a < argc; ++a) {
            std::string k = argv[a];
            auto val = [&]() -> std::string {
                if (a + 1 >= argc) throw std::runtime_error("missing value for " + k);
                return argv[++a];
            };
            if (k == "--time") prm.timeLimit = std::stod(val());
            else if (k == "--seed") prm.seed = std::stoull(val());
            else if (k == "--threads") prm.threads = std::stoi(val());
            else if (k == "--init") initPath = val();
            else if (k == "--out") outArg = val();
            else if (k == "--K") K = std::stoi(val());
            else if (k == "--idle") idleW = std::stod(val());
            else if (k == "--seg") prm.ls.maxSeg = std::stoi(val());
            else if (k == "--cross") prm.ls.maxCross = std::stoi(val());
            else if (k == "--first") prm.ls.firstImprovement = std::stoi(val()) != 0;
            else if (k == "--T0") prm.T0 = std::stod(val());
            else if (k == "--Tf") prm.Tf = std::stod(val());
            else if (k == "--kmin") prm.pert.kmin = std::stoi(val());
            else if (k == "--kmax") prm.pert.kmax = std::stoi(val());
            else if (k == "--pwin") prm.pert.pWindow = std::stod(val());
            else if (k == "--maxrw") prm.pert.maxRunways = std::stoi(val());
            else if (k == "--inswin") prm.pert.insWin = std::stoi(val());
            else if (k == "--blink") prm.pert.blink = std::stod(val());
            else if (k == "--pdelay") prm.pert.pDelayedSeed = std::stod(val());
            else if (k == "--reset") prm.resetFactor = std::stod(val());
            else if (k == "--sync") prm.syncPeriod = std::stod(val());
            else if (k == "--cycle") prm.cycle = std::stod(val());
            else if (k == "--report") prm.reportEvery = std::stod(val());
            else if (k == "--check") prm.debugCheck = true;
            else if (k == "--maxiter") prm.maxIter = std::stoll(val());
            else if (k == "--prune") prm.ls.prune = std::stoi(val()) != 0;
            else if (k == "--assign") prm.assignMode = std::stoi(val());
            else if (k == "--selftest") selftest = true;
            else if (k == "--sweep") sweep = true;
            else if (k == "--donor") donors.push_back(val());
            else if (k == "--quiet") prm.verbose = false;
            else throw std::runtime_error("unknown option " + k);
        }
        if (prm.timeLimit <= 0 || prm.threads < 1 || K < 1 || prm.T0 <= 0 || prm.Tf <= 0 ||
            prm.pert.kmin < 1 || prm.pert.kmax < prm.pert.kmin)
            throw std::runtime_error("invalid parameter value");
        Timer total;
        const Instance in = loadInstance(instPath);
        const Neighbors nb = buildNeighbors(in, K, idleW);
        if (selftest) return selfTest(instPath, prm, nb, in);

        if (!outArg.empty()) prm.outPath = isDir(outArg) ? outArg + "/" + in.name + "_gls.txt" : outArg;
        std::vector<std::vector<int>> init =
            initPath.empty() ? greedyConstruct(in) : readSolutionFile(in, initPath);
        if (sweep) return sweepTest(in, nb, prm, init);
        if (!donors.empty()) {
            if (!outArg.empty()) prm.outPath = outArg;
            return recombineTest(in, nb, prm, init, donors);
        }
        std::printf("instance %s n=%d m=%d  init=%s cost=%lld  (setup %.2fs)\n", in.name.c_str(), in.n, in.m,
                    initPath.empty() ? "greedy" : initPath.c_str(), referenceCost(in, init), total.seconds());
        prm.timeLimit = std::max(0.1, prm.timeLimit - total.seconds() - 0.05);
        SharedBest sb;
        runILS(in, nb, prm, init, sb);
        const ll ref = referenceCost(in, sb.seqs);
        std::printf("FINAL cost=%lld reference=%lld time=%.1fs out=%s\n", sb.cost, ref, total.seconds(),
                    prm.outPath.c_str());
        if (ref != sb.cost) {
            std::fprintf(stderr, "internal cost mismatch!\n");
            return 1;
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
