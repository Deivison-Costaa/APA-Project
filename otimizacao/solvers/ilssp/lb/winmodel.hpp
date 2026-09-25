// Window relaxation for P | r_j, s_ij | sum p_j (S_j - r_j).
//
// Split time into windows W = [a, b). The cost of job j splits exactly into
// per-window contributions p_j * |[r_j, S_j) ∩ W|. Restricted to the jobs that
// START inside W, every runway is a contiguous block of its true sequence, so
// the following is a relaxation of the contribution of W:
//   * "home" jobs (a <= r_j < b) start in W (cost p_j (S_j - r_j)) or are pushed
//     past b (cost p_j (b - r_j));
//   * "early" jobs (r_j < a) may be pulled into W (cost p_j (S_j - a)) or not (0);
//   * the first job of each runway in W starts at max(r_j, a) (no predecessor).
// If a solution X had total cost < UB = sum_W UB_W (the incumbent's per-window
// contributions), some window would have contribution < UB_W; so if for every
// window the relaxation has no schedule of cost <= UB_W - 1, the incumbent is
// optimal. That budget also bounds the start-time states of every job.
//
// The relaxation is solved as an arc-time-indexed flow MIP: nodes (job, start),
// transitions (i,s) -> (j, max(r_j, s + c_i + t_ij)), plus a "free runway"
// time chain for transitions whose setup can no longer bind.
#pragma once

#include <string>
#include <vector>

#include "../src/instance.hpp"

struct WindowSpec {
    int a = 0;
    int b = 0;
    long long ub = 0;  // incumbent contribution of this window
};

struct WindowResult {
    bool built = false;
    int homeJobs = 0, earlyJobs = 0, states = 0, arcs = 0;
    int status = 0;
    bool infeasible = false;
    double primal = 0.0;   // best window schedule found (relaxation)
    double dual = 0.0;     // proven lower bound of the relaxation
    double seconds = 0.0;
    double lpBound = 0.0;    // LP relaxation value (interior point, no crossover)
    double lpSeconds = 0.0;
    bool certifiedByLp = false;
    bool certified = false;  // no window schedule of cost <= ub - 1
    std::vector<int> pulledIn;   // early jobs used by the best relaxation schedule
    std::vector<int> pushedOut;  // home jobs pushed past b
    std::string diag;            // LP diagnostics (fractional costs per job)
    double rcBound = 0.0;        // Lagrangian bound from the LP duals (reduced cost fixing)
    int rcKept = 0;              // columns surviving reduced cost fixing
};

// globalUb: incumbent total cost (bounds which early jobs can be pulled in).
// lpFirst: solve the LP relaxation with interior point first and skip the MIP
// when LP > ub - 1 already certifies the window.
// rcFix: drop columns whose reduced cost proves they cannot appear in a schedule of cost <= ub - 1,
// then solve the MIP on the smaller model with objective_bound ub - 0.5.
WindowResult solveWindow(const Instance& ins, const WindowSpec& w, long long globalUb, double timeLimit,
                         bool verbose, bool lpFirst = true, bool diagnose = false, bool runMip = true,
                         bool rcFix = false);
