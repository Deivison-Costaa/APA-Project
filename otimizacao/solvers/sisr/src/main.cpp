// CLI: sisr <instance> [--time s] [--seed k] [--threads k] [--init file] [--out file|dir] [tuning...]
#include <sys/stat.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "sisr.hpp"

namespace {

struct Cli {
    std::string instance, init, out;
    int threads = 1;
    Params P;
};

void usage() {
    std::fprintf(stderr,
                 "usage: sisr <instance> [--time s] [--seed k] [--threads k] [--init sol] [--out file|dir]\n"
                 "  tuning: --cbar x --lmax k --split x --keep x --blink x --window k --T0 x --Tf x --adj k --tail x\n          --bias x --slice x --slicew k\n          --global frac --whalf k --witers k --wT0 x --wTf x --wspread x --hot x --opt x\n          --cgap k --cpow x --margin lo,hi --wfix lo,hi\n"
                 "          --order wRand,wRAsc,wPDesc,wRDesc --report s --quiet --selfcheck\n");
}

Cli parse(int argc, char** argv) {
    Cli c;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto val = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + a);
            return argv[++i];
        };
        if (a == "--time") c.P.timeLimit = std::stod(val());
        else if (a == "--seed") c.P.seed = std::stoull(val());
        else if (a == "--threads") c.threads = std::max(1, std::stoi(val()));
        else if (a == "--init") c.init = val();
        else if (a == "--out") c.out = val();
        else if (a == "--cbar") c.P.cbar = std::stod(val());
        else if (a == "--lmax") c.P.lmax = std::stoi(val());
        else if (a == "--split") c.P.splitProb = std::stod(val());
        else if (a == "--keep") c.P.keepProb = std::stod(val());
        else if (a == "--blink") c.P.blink = std::stod(val());
        else if (a == "--window") c.P.window = std::stoi(val());
        else if (a == "--T0") c.P.T0 = std::stod(val());
        else if (a == "--Tf") c.P.Tf = std::stod(val());
        else if (a == "--adj") c.P.adjK = std::stoi(val());
        else if (a == "--tail") c.P.tailProb = std::stod(val());
        else if (a == "--global") c.P.globalFrac = std::stod(val());
        else if (a == "--whalf") c.P.wHalf = std::stoi(val());
        else if (a == "--witers") c.P.wIters = std::stoll(val());
        else if (a == "--wT0") c.P.wT0 = std::stod(val());
        else if (a == "--wTf") c.P.wTf = std::stod(val());
        else if (a == "--wspread") c.P.wSpread = std::stod(val());
        else if (a == "--hot") c.P.hotProb = std::stod(val());
        else if (a == "--opt") c.P.optProb = std::stod(val());
        else if (a == "--cgap") c.P.clusterGap = std::stoi(val());
        else if (a == "--cpow") c.P.clusterPow = std::stod(val());
        else if (a == "--margin") {
            std::string v = val();
            if (std::sscanf(v.c_str(), "%d,%d", &c.P.marginMin, &c.P.marginMax) != 2) throw std::runtime_error("bad --margin");
        }
        else if (a == "--wfix") {
            std::string v = val();
            if (std::sscanf(v.c_str(), "%d,%d", &c.P.wFixLo, &c.P.wFixHi) != 2) throw std::runtime_error("bad --wfix");
        }
        else if (a == "--bias") c.P.seedBias = std::stod(val());
        else if (a == "--slice") c.P.sliceProb = std::stod(val());
        else if (a == "--slicew") c.P.sliceW = std::stoi(val());
        else if (a == "--report") c.P.reportEvery = std::stod(val());
        else if (a == "--quiet") c.P.verbose = 0;
        else if (a == "--selfcheck") c.P.selfCheck = true;
        else if (a == "--order") {
            std::string v = val();
            if (std::sscanf(v.c_str(), "%d,%d,%d,%d", &c.P.wRandom, &c.P.wRAsc, &c.P.wPDesc, &c.P.wRDesc) != 4)
                throw std::runtime_error("bad --order " + v);
        } else if (!a.empty() && a[0] == '-') throw std::runtime_error("unknown option " + a);
        else if (c.instance.empty()) c.instance = a;
        else throw std::runtime_error("unexpected argument " + a);
    }
    if (c.instance.empty()) throw std::runtime_error("missing instance");
    if (c.P.T0 <= 0 || c.P.Tf <= 0 || c.P.wT0 <= 0 || c.P.wTf <= 0) throw std::runtime_error("temperatures must be > 0");
    return c;
}

bool isDir(const std::string& p) {
    struct stat sb;
    return stat(p.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}

std::string outPath(const Cli& c, const Instance& I, long long cost) {
    if (c.out.empty()) return I.name + "_sisr.txt";
    if (isDir(c.out)) return c.out + "/" + I.name + "_" + std::to_string(cost) + "_sisr.txt";
    return c.out;
}

}  // namespace

int main(int argc, char** argv) {
    Cli cli;
    try {
        cli = parse(argc, argv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        usage();
        return 2;
    }
    const double t0 = nowSeconds();
    Instance I;
    Seqs initSeqs;
    try {
        I = readInstance(cli.instance);
        if (!cli.init.empty()) initSeqs = readSolutionFile(I, cli.init);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 2;
    }
    std::printf("instance %s n=%d m=%d time=%.1fs threads=%d seed=%llu\n", I.name.c_str(), I.n, I.m,
                cli.P.timeLimit, cli.threads, static_cast<unsigned long long>(cli.P.seed));

    SharedBest shared;
    std::vector<std::unique_ptr<SisrSolver>> solvers;
    for (int k = 0; k < cli.threads; ++k) {
        solvers.push_back(std::make_unique<SisrSolver>(I, cli.P, k));
        if (!initSeqs.empty()) solvers.back()->setInitial(initSeqs);
        else solvers.back()->construct();
    }
    std::printf("initial cost %lld (%s) after %.2fs\n", solvers[0]->bestCost(),
                initSeqs.empty() ? "greedy" : "warm start", nowSeconds() - t0);
    std::fflush(stdout);

    std::vector<RunStats> stats(cli.threads);
    std::vector<std::thread> pool;
    for (int k = 0; k < cli.threads; ++k)
        pool.emplace_back([&, k]() { stats[k] = solvers[k]->run(shared, t0); });
    // Checkpoint: when --out is a file, rewrite it every few seconds if the best improved,
    // so an externally killed run still leaves its best solution.
    std::atomic<bool> finished{false};
    std::thread writer([&]() {
        if (cli.out.empty() || isDir(cli.out)) return;
        long long written = kInf;
        while (!finished.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (nowSeconds() - t0 < 10.0 || shared.cost.load() >= written) continue;
            Seqs snap;
            long long c;
            {
                std::lock_guard<std::mutex> lk(shared.mtx);
                snap = shared.seqs;
                c = shared.cost.load();
            }
            if (evaluate(I, snap) == c && writeSolutionFile(I, snap, c, cli.out)) written = c;
            for (int w = 0; w < 10 && !finished.load(); ++w) std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    for (auto& th : pool) th.join();
    finished.store(true);
    writer.join();

    long long totalIt = 0;
    for (const auto& s : stats) totalIt += s.iters;
    const long long best = shared.cost.load();
    const long long check = evaluate(I, shared.seqs);
    if (check != best) {
        std::fprintf(stderr, "INTERNAL ERROR: best cost %lld but re-evaluation gives %lld\n", best, check);
        return 3;
    }
    const std::string path = outPath(cli, I, best);
    if (!writeSolutionFile(I, shared.seqs, best, path)) {
        std::fprintf(stderr, "error: cannot write %s\n", path.c_str());
        return 4;
    }
    std::printf("final best %lld  iters=%lld (%.0f it/s total)  written to %s\n", best, totalIt,
                totalIt / std::max(1e-9, stats[0].seconds), path.c_str());
    return 0;
}
