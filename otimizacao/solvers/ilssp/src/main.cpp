// ILS-SP solver for P | r_j, s_ij | sum p_j (S_j - r_j) (Copa APA).
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

#include "adapt.hpp"
#include "construct.hpp"
#include "driver.hpp"

namespace fs = std::filesystem;

namespace {

double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

std::string outputPath(const Options& o) {
    const std::string name = fs::path(o.instance).stem().string() + "_ilssp.txt";
    if (o.out.empty()) return name;
    if (fs::is_directory(o.out)) return (fs::path(o.out) / name).string();
    return o.out;
}

}  // namespace

int main(int argc, char** argv) {
    Options opt = parseOptions(argc, argv);
    const double tStart = nowSec();
    Instance ins;
    try {
        ins = loadInstance(opt.instance);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    const std::string outFile = outputPath(opt);
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    std::printf("instance %s n=%d m=%d time=%.0fs seed=%llu threads=%d sp=%d out=%s\n", opt.instance.c_str(),
                ins.n, ins.m, opt.timeLimit, static_cast<unsigned long long>(opt.seed), opt.threads,
                opt.useSp ? 1 : 0, outFile.c_str());

    Rng rng(opt.seed);
    Solution start = constructGreedy(ins, rng, 0.0);
    std::printf("greedy cost %lld\n", start.cost);
    if (!opt.initFile.empty()) {
        std::vector<std::vector<int>> runways;
        long long claimed = 0;
        std::string err;
        if (!readSolutionFile(opt.initFile, ins, runways, claimed, err)) {
            std::fprintf(stderr, "error reading --init: %s\n", err.c_str());
            return 1;
        }
        start = solutionFromRunways(ins, runways);
        std::printf("init cost %lld (claimed %lld)\n", start.cost, claimed);
    }

    const AdaptResult ad = adaptWindows(ins, start, opt);
    std::printf("congestion: %.0f%% delayed after local search -> %s windows%s\n", 100.0 * ad.delayedFrac,
                ad.wide ? "wide" : "narrow", ad.applied ? "" : " (not applied: fixed on the command line)");
    Driver driver(ins, opt);
    const Solution best = driver.solve(start, tStart, tStart + opt.timeLimit, outFile);
    std::string err;
    if (!validateSolution(ins, best, err)) {
        std::fprintf(stderr, "internal error at end: %s\n", err.c_str());
        return 1;
    }
    if (!writeSolutionFile(outFile, best)) {
        std::fprintf(stderr, "error: cannot write %s\n", outFile.c_str());
        return 1;
    }
    std::printf("wrote %s cost %lld\n", outFile.c_str(), best.cost);
    return 0;
}
