#include "winmodel.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#include "../src/highs_min.h"

namespace {

double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

constexpr int kSrc = -1;
constexpr int kSink = -2;

struct Arc {
    int from;   // node id, or kSrc
    int to;     // node id, or kSink (-2) / -3 for a push variable
    int job;    // candidate index whose coverage row gets +1, or -1
    double cost;
};

struct Model {
    std::vector<int> cand;      // candidate jobs
    std::vector<char> home;     // per candidate
    std::vector<int> lo, hi, base;
    int nStates = 0;
    int nF = 0;                 // free-runway chain nodes, times a..b-1
    std::vector<Arc> arcs;
};

Model buildModel(const Instance& ins, const WindowSpec& w, long long globalUb) {
    Model md;
    const long long budget = w.ub - 1;
    std::vector<int> candOf(ins.n, -1);
    for (int j = 0; j < ins.n; ++j) {
        const bool isHome = ins.r[j] >= w.a && ins.r[j] < w.b;
        const bool isEarly = ins.r[j] < w.a && static_cast<long long>(ins.p[j]) * (w.a - ins.r[j]) <= globalUb - 1;
        if (!isHome && !isEarly) continue;
        const int lo = std::max(ins.r[j], w.a);
        const long long span = ins.p[j] > 0 ? budget / ins.p[j] : (w.b - 1 - lo);
        const int hi = static_cast<int>(std::min<long long>(w.b - 1, lo + span));
        if (hi < lo) continue;
        candOf[j] = static_cast<int>(md.cand.size());
        md.cand.push_back(j);
        md.home.push_back(isHome);
        md.lo.push_back(lo);
        md.hi.push_back(hi);
        md.base.push_back(md.nStates);
        md.nStates += hi - lo + 1;
    }
    md.nF = w.b - w.a;
    const int K = static_cast<int>(md.cand.size());
    auto node = [&](int k, int s) { return md.base[k] + (s - md.lo[k]); };
    auto fNode = [&](int tau) { return md.nStates + (tau - w.a); };
    std::vector<int> maxT(K, 0);
    for (int i = 0; i < K; ++i)
        for (int k = 0; k < K; ++k)
            if (k != i) maxT[i] = std::max(maxT[i], ins.sep(md.cand[i], md.cand[k]));

    for (int k = 0; k < K; ++k) md.arcs.push_back({kSrc, node(k, md.lo[k]), k, 0.0});
    for (int i = 0; i < K; ++i) {
        const int fi = md.cand[i];
        for (int s = md.lo[i]; s <= md.hi[i]; ++s) {
            const int from = node(i, s);
            md.arcs.push_back({from, kSink, -1, 0.0});
            const int end = s + ins.c[fi];
            for (int k = 0; k < K; ++k) {
                if (k == i) continue;
                const int fj = md.cand[k];
                const int earliest = end + ins.sep(fi, fj);
                int sj;
                double cost;
                if (md.home[k]) {
                    if (ins.r[fj] >= end + maxT[i]) continue;  // routed through the free chain
                    sj = std::max(ins.r[fj], earliest);
                    cost = static_cast<double>(ins.p[fj]) * (sj - ins.r[fj]);
                } else {
                    sj = std::max(earliest, w.a);
                    cost = static_cast<double>(ins.p[fj]) * (sj - w.a);
                }
                if (sj < md.lo[k] || sj > md.hi[k]) continue;
                md.arcs.push_back({from, node(k, sj), k, cost});
            }
            const int tau = end + maxT[i];
            if (tau < w.b) md.arcs.push_back({from, fNode(std::max(tau, w.a)), -1, 0.0});
        }
    }
    for (int tau = w.a; tau + 1 < w.b; ++tau) md.arcs.push_back({fNode(tau), fNode(tau + 1), -1, 0.0});
    for (int k = 0; k < K; ++k)
        if (md.home[k]) md.arcs.push_back({fNode(ins.r[md.cand[k]]), node(k, ins.r[md.cand[k]]), k, 0.0});
    for (int k = 0; k < K; ++k) {
        if (!md.home[k]) continue;
        const long long push = static_cast<long long>(ins.p[md.cand[k]]) * (w.b - ins.r[md.cand[k]]);
        if (push <= budget) md.arcs.push_back({kSrc, -3, k, static_cast<double>(push)});
    }
    return md;
}

// Column-wise LP data shared by the LP and MIP solves.
struct LpData {
    std::vector<double> cost, lower, upper, rowLo, rowUp, val;
    std::vector<HighsInt> start, index, integ;
    int nCols() const { return static_cast<int>(cost.size()); }
};

// Lagrangian bound for ANY row multipliers y (column lower bounds are all 0):
//   L(y) = sum_i (y_i >= 0 ? y_i lo_i : y_i up_i) + sum_j min(0, d_j) u_j,  d = c - A^T y.
// Setting a column with d_j > 0 to 1 raises the bound to L + d_j, so when L + d_j > budget
// no solution of cost <= budget uses that column. Validity does not depend on y being optimal,
// so the approximate interior-point duals are safe to use.
struct RcFix {
    double bound = -1e300;
    std::vector<char> keep;
    int kept = 0;
};

RcFix reducedCostFix(const LpData& lp, const std::vector<double>& y, double budget) {
    RcFix best;
    const int nc = lp.nCols();
    for (const double sign : {1.0, -1.0}) {  // HiGHS' row dual sign convention is checked, not assumed
        double L = 0.0;
        for (size_t i = 0; i < y.size(); ++i) {
            const double yi = sign * y[i];
            L += yi >= 0 ? yi * lp.rowLo[i] : yi * lp.rowUp[i];
        }
        std::vector<double> d(nc);
        for (int j = 0; j < nc; ++j) {
            double dj = lp.cost[j];
            for (HighsInt k = lp.start[j]; k < lp.start[j + 1]; ++k) dj -= lp.val[k] * sign * y[lp.index[k]];
            d[j] = dj;
            if (dj < 0) L += dj * lp.upper[j];
        }
        if (L <= best.bound) continue;
        best.bound = L;
        best.keep.assign(nc, 1);
        best.kept = nc;
        const double tol = 1e-3 + 1e-9 * std::abs(L);  // keeping extra columns is always safe
        for (int j = 0; j < nc; ++j)
            if (d[j] > 0 && L + d[j] > budget + tol) {
                best.keep[j] = 0;
                --best.kept;
            }
    }
    return best;
}

LpData restrictColumns(const LpData& lp, const std::vector<char>& keep, std::vector<int>& colMap) {
    LpData out;
    out.rowLo = lp.rowLo;
    out.rowUp = lp.rowUp;
    colMap.clear();
    for (int j = 0; j < lp.nCols(); ++j) {
        if (!keep[j]) continue;
        colMap.push_back(j);
        out.start.push_back(static_cast<HighsInt>(out.index.size()));
        out.cost.push_back(lp.cost[j]);
        out.lower.push_back(lp.lower[j]);
        out.upper.push_back(lp.upper[j]);
        out.integ.push_back(lp.integ[j]);
        for (HighsInt k = lp.start[j]; k < lp.start[j + 1]; ++k) {
            out.index.push_back(lp.index[k]);
            out.val.push_back(lp.val[k]);
        }
    }
    out.start.push_back(static_cast<HighsInt>(out.index.size()));
    return out;
}

}  // namespace

WindowResult solveWindow(const Instance& ins, const WindowSpec& w, long long globalUb, double timeLimit,
                         bool verbose, bool lpFirst, bool diagnose, bool runMip, bool rcFix) {
    WindowResult res;
    if (w.ub <= 0) {
        res.certified = true;
        return res;
    }
    const double t0 = nowSec();
    const Model md = buildModel(ins, w, globalUb);
    const int K = static_cast<int>(md.cand.size());
    const int nNodes = md.nStates + md.nF;
    const int rowCover = nNodes, rowSrc = nNodes + K, nRows = nNodes + K + 1;
    res.built = true;
    res.states = md.nStates;
    res.arcs = static_cast<int>(md.arcs.size());
    for (int k = 0; k < K; ++k) (md.home[k] ? res.homeJobs : res.earlyJobs)++;

    LpData lp;
    lp.rowLo.assign(nRows, 0.0);
    lp.rowUp.assign(nRows, 0.0);
    for (const Arc& e : md.arcs) {
        lp.start.push_back(static_cast<HighsInt>(lp.index.size()));
        lp.cost.push_back(e.cost);
        lp.lower.push_back(0.0);
        const bool chain = e.from >= md.nStates && e.to >= md.nStates;  // free-runway chain
        lp.upper.push_back(chain ? ins.m : 1.0);
        lp.integ.push_back(chain ? 0 : kHighsVarTypeInteger);
        std::vector<std::pair<int, double>> ent;
        if (e.from >= 0) ent.push_back({e.from, -1.0});
        if (e.from == kSrc && e.to != -3) ent.push_back({rowSrc, 1.0});
        if (e.to >= 0) ent.push_back({e.to, 1.0});
        if (e.job >= 0) ent.push_back({rowCover + e.job, 1.0});
        std::sort(ent.begin(), ent.end());
        for (auto& [r, v] : ent) {
            lp.index.push_back(r);
            lp.val.push_back(v);
        }
    }
    lp.start.push_back(static_cast<HighsInt>(lp.index.size()));
    for (int k = 0; k < K; ++k) {
        lp.rowLo[rowCover + k] = md.home[k] ? 1.0 : 0.0;
        lp.rowUp[rowCover + k] = 1.0;
    }
    lp.rowLo[rowSrc] = 0.0;
    lp.rowUp[rowSrc] = ins.m;

    const HighsInt nc = lp.nCols();
    std::vector<double> rowDual;
    if (lpFirst) {
        const double tl = nowSec();
        void* hl = Highs_create();
        Highs_setBoolOptionValue(hl, "output_flag", verbose ? 1 : 0);
        Highs_setIntOptionValue(hl, "threads", 1);
        Highs_setStringOptionValue(hl, "solver", "ipm");
        Highs_setStringOptionValue(hl, "run_crossover", "off");
        Highs_setDoubleOptionValue(hl, "time_limit", timeLimit);
        Highs_passLp(hl, nc, nRows, static_cast<HighsInt>(lp.index.size()), kHighsMatrixFormatColwise,
                     kHighsObjSenseMinimize, 0.0, lp.cost.data(), lp.lower.data(), lp.upper.data(),
                     lp.rowLo.data(), lp.rowUp.data(), lp.start.data(), lp.index.data(), lp.val.data());
        Highs_run(hl);
        const HighsInt st = Highs_getModelStatus(hl);
        res.lpBound = st == kHighsModelStatusOptimal ? Highs_getObjectiveValue(hl) : -1.0;
        if (st == 8) res.lpBound = 1e18;  // LP infeasible => no window schedule within budget
        if (st == kHighsModelStatusOptimal && (diagnose || rcFix)) {
            std::vector<double> x(nc), cd(nc), rv(nRows), rd(nRows);
            Highs_getSolution(hl, x.data(), cd.data(), rv.data(), rd.data());
            if (rcFix) rowDual = rd;
            if (diagnose) {
                std::vector<double> jobCost(K, 0.0), jobIn(K, 0.0);
                double pushCost = 0.0, earlyCost = 0.0;
                for (HighsInt j = 0; j < nc; ++j) {
                    const Arc& e = md.arcs[j];
                    if (x[j] < 1e-6 || e.job < 0) continue;
                    jobCost[e.job] += e.cost * x[j];
                    jobIn[e.job] += x[j];
                    if (e.to == -3) pushCost += e.cost * x[j];
                    else if (!md.home[e.job]) earlyCost += e.cost * x[j];
                }
                char buf[160];
                std::snprintf(buf, sizeof buf,
                              "   LP split: push-out %.1f, pulled-in early jobs %.1f, home delays %.1f\n", pushCost,
                              earlyCost, res.lpBound - pushCost - earlyCost);
                res.diag += buf;
                for (int k = 0; k < K; ++k) {
                    if (jobCost[k] < 0.5 && (md.home[k] || jobIn[k] < 0.05)) continue;
                    std::snprintf(buf, sizeof buf, "   job %d %s r=%d p=%d in=%.2f cost=%.1f\n", md.cand[k] + 1,
                                  md.home[k] ? "home " : "EARLY", ins.r[md.cand[k]], ins.p[md.cand[k]], jobIn[k],
                                  jobCost[k]);
                    res.diag += buf;
                }
            }
        }
        Highs_destroy(hl);
        res.lpSeconds = nowSec() - tl;
        // 0.01 margin covers the interior-point tolerance (costs are integers, the gap to prove is 1)
        if (res.lpBound > static_cast<double>(w.ub - 1) + 0.01) {
            res.certified = res.certifiedByLp = true;
            res.dual = res.lpBound;
            res.status = st;
            res.seconds = nowSec() - t0;
            return res;
        }
    }
    if (!runMip) {
        res.seconds = nowSec() - t0;
        return res;
    }
    std::vector<int> colMap;
    LpData mip;
    const LpData* use = &lp;
    if (!rowDual.empty()) {
        const RcFix fx = reducedCostFix(lp, rowDual, static_cast<double>(w.ub - 1));
        res.rcBound = fx.bound;
        res.rcKept = fx.kept;
        std::printf("   rcfix: Lagrangian bound %.3f, kept %d of %d columns (%.1fs)\n", fx.bound, fx.kept,
                    static_cast<int>(nc), nowSec() - t0);
        std::fflush(stdout);
        if (fx.bound > static_cast<double>(w.ub - 1) + 0.01) {
            res.certified = res.certifiedByLp = true;
            res.dual = fx.bound;
            res.seconds = nowSec() - t0;
            return res;
        }
        mip = restrictColumns(lp, fx.keep, colMap);
        lp = LpData();  // release the full model before the MIP
        use = &mip;
    }
    const HighsInt mc = use->nCols();
    void* h = Highs_create();
    Highs_setBoolOptionValue(h, "output_flag", verbose ? 1 : 0);
    Highs_setIntOptionValue(h, "threads", 1);
    Highs_setDoubleOptionValue(h, "time_limit", timeLimit);
    Highs_setDoubleOptionValue(h, "mip_rel_gap", 0.0);
    Highs_setDoubleOptionValue(h, "mip_abs_gap", 0.5);
    // only schedules of cost <= ub - 1 matter: prune everything above (infeasible => certified)
    if (rcFix && !std::getenv("WINLB_NO_OBJBOUND"))
        Highs_setDoubleOptionValue(h, "objective_bound", static_cast<double>(w.ub) - 0.5);
    Highs_passMip(h, mc, nRows, static_cast<HighsInt>(use->index.size()), kHighsMatrixFormatColwise,
                  kHighsObjSenseMinimize, 0.0, use->cost.data(), use->lower.data(), use->upper.data(),
                  use->rowLo.data(), use->rowUp.data(), use->start.data(), use->index.data(), use->val.data(),
                  use->integ.data());
    Highs_run(h);
    res.status = Highs_getModelStatus(h);
    res.infeasible = (res.status == 8);  // kHighsModelStatusInfeasible
    Highs_getDoubleInfoValue(h, "mip_dual_bound", &res.dual);
    HighsInt pss = 0;
    Highs_getIntInfoValue(h, "primal_solution_status", &pss);
    if (pss == 2) {
        res.primal = Highs_getObjectiveValue(h);
        std::vector<double> x(mc), cd(mc), rv(nRows), rd(nRows);
        Highs_getSolution(h, x.data(), cd.data(), rv.data(), rd.data());
        for (HighsInt j = 0; j < mc; ++j) {
            if (x[j] < 0.5) continue;
            const Arc& e = md.arcs[colMap.empty() ? j : colMap[j]];
            if (e.to == -3) res.pushedOut.push_back(md.cand[e.job] + 1);
            else if (e.job >= 0 && !md.home[e.job]) res.pulledIn.push_back(md.cand[e.job] + 1);
        }
    } else {
        res.primal = -1;
    }
    Highs_destroy(h);
    res.certified = res.infeasible || res.dual > static_cast<double>(w.ub - 1) + 1e-4;
    res.seconds = nowSec() - t0;
    return res;
}
