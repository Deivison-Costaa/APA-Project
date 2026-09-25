// winlb: proves (or bounds) optimality of an incumbent by per-window relaxations.
// usage: winlb <instance> <solution> [--margin M] [--gap G] [--time T] [--window a:b]... [--verbose]
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/solution.hpp"
#include "winmodel.hpp"

namespace {

struct Interval {
    int lo, hi;
};

std::vector<WindowSpec> autoWindows(const std::vector<Interval>& delays, int margin, int gap, int horizon) {
    std::vector<Interval> iv = delays;
    std::sort(iv.begin(), iv.end(), [](const Interval& x, const Interval& y) { return x.lo < y.lo; });
    std::vector<Interval> clusters;
    for (const Interval& d : iv) {
        if (!clusters.empty() && d.lo <= clusters.back().hi + gap) clusters.back().hi = std::max(clusters.back().hi, d.hi);
        else clusters.push_back(d);
    }
    std::vector<WindowSpec> ws;
    for (const Interval& c : clusters) {
        WindowSpec w;
        w.a = std::max(0, c.lo - margin);
        w.b = std::min(horizon, c.hi + margin);
        if (!ws.empty() && w.a < ws.back().b) {
            // split the overlap in the middle of the quiet gap between the two clusters
            const int mid = (ws.back().b - margin + w.a + margin) / 2;
            ws.back().b = mid;
            w.a = mid;
        }
        ws.push_back(w);
    }
    return ws;
}

long long contribution(const Instance& ins, const std::vector<int>& S, int a, int b) {
    long long tot = 0;
    for (int j = 0; j < ins.n; ++j) {
        const int lo = std::max(ins.r[j], a), hi = std::min(S[j], b);
        if (hi > lo) tot += static_cast<long long>(ins.p[j]) * (hi - lo);
    }
    return tot;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: winlb <instance> <solution> [--margin M] [--gap G] [--time T] "
                             "[--window a:b]... [--only k] [--verbose] [--diag] [--no-mip] [--rcfix]\n");
        return 2;
    }
    const Instance ins = loadInstance(argv[1]);
    std::vector<std::vector<int>> runways;
    long long claimed = 0;
    std::string err;
    if (!readSolutionFile(argv[2], ins, runways, claimed, err)) {
        std::fprintf(stderr, "bad solution: %s\n", err.c_str());
        return 1;
    }
    int margin = 150, gap = -1, only = -1;
    double timeLimit = 120;
    bool verbose = false, diagnose = false, runMip = true, rcFix = false;
    std::vector<WindowSpec> manual;
    for (int i = 3; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--margin") margin = std::atoi(argv[++i]);
        else if (a == "--gap") gap = std::atoi(argv[++i]);
        else if (a == "--time") timeLimit = std::atof(argv[++i]);
        else if (a == "--only") only = std::atoi(argv[++i]);
        else if (a == "--verbose") verbose = true;
        else if (a == "--diag") diagnose = true;
        else if (a == "--no-mip") runMip = false;
        else if (a == "--rcfix") rcFix = true;
        else if (a == "--window") {
            WindowSpec w;
            if (std::sscanf(argv[++i], "%d:%d", &w.a, &w.b) != 2) return 2;
            manual.push_back(w);
        }
    }
    if (gap < 0) gap = 2 * margin;
    const Solution sol = solutionFromRunways(ins, runways);
    std::vector<int> S(ins.n);
    std::vector<Interval> delays;
    int horizon = 0;
    for (int j = 0; j < ins.n; ++j) {
        S[j] = sol.startOf[j];
        horizon = std::max(horizon, S[j] + ins.c[j] + 1);
        if (S[j] > ins.r[j]) delays.push_back({ins.r[j], S[j]});
    }
    std::vector<WindowSpec> ws = manual.empty() ? autoWindows(delays, margin, gap, horizon) : manual;
    long long covered = 0;
    for (auto& w : ws) {
        w.ub = contribution(ins, S, w.a, w.b);
        covered += w.ub;
    }
    std::printf("instance %s incumbent %lld, %zu windows, windows cover %lld of the cost\n", argv[1], sol.cost,
                ws.size(), covered);
    if (covered != sol.cost) std::printf("WARNING: windows do not cover all delay; outside part counted as LB 0\n");
    long long lbSum = 0;
    int certified = 0;
    for (size_t k = 0; k < ws.size(); ++k) {
        if (only >= 0 && static_cast<int>(k) != only) continue;
        const WindowSpec& w = ws[k];
        const WindowResult r = solveWindow(ins, w, sol.cost, timeLimit, verbose, true, diagnose, runMip, rcFix);
        const long long lbw = r.infeasible ? w.ub : std::min<long long>(w.ub, static_cast<long long>(std::ceil(r.dual - 1e-6)));
        lbSum += std::max(0LL, lbw);
        certified += r.certified;
        std::printf("window %zu [%d,%d) ub %lld | home %d early %d states %d arcs %d | LP %.3f (%.1fs) | "
                    "status %d primal %.0f dual %.2f | %.1fs | %s\n",
                    k, w.a, w.b, w.ub, r.homeJobs, r.earlyJobs, r.states, r.arcs, r.lpBound, r.lpSeconds,
                    r.status, r.primal, r.dual, r.seconds,
                    r.certified ? (r.certifiedByLp ? "CERTIFIED (LP)" : "CERTIFIED (MIP)") : "open");
        if (!r.certified) std::printf("%s", r.diag.c_str());
        if (!r.certified && (!r.pulledIn.empty() || !r.pushedOut.empty())) {
            std::printf("   pulled-in:");
            for (int f : r.pulledIn) std::printf(" %d", f);
            std::printf("   pushed-out:");
            for (int f : r.pushedOut) std::printf(" %d", f);
            std::printf("\n");
        }
    }
    std::printf("SUMMARY: incumbent %lld, certified windows %d/%zu, lower bound %lld%s\n", sol.cost, certified,
                ws.size(), lbSum + (sol.cost - covered) * 0,
                (only < 0 && certified == static_cast<int>(ws.size()) && covered == sol.cost) ? "  => OPTIMAL" : "");
    return 0;
}
