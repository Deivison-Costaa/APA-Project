#include "spsolver.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <unordered_set>

#include "highs_min.h"

namespace {

double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

struct Model {
    std::vector<double> cost, lower, upper, rowLower, rowUpper, value;
    std::vector<HighsInt> start, index, integrality;
};

Model buildModel(const Instance& ins, const ColumnPool& pool, const std::vector<int>& colIds) {
    Model md;
    const size_t nc = colIds.size();
    md.cost.reserve(nc);
    md.start.reserve(nc + 1);
    std::vector<int> rows;
    for (int id : colIds) {
        const Column& c = pool.col(id);
        md.start.push_back(static_cast<HighsInt>(md.index.size()));
        md.cost.push_back(static_cast<double>(c.cost));
        rows.assign(c.seq.begin(), c.seq.end());
        std::sort(rows.begin(), rows.end());
        for (int f : rows) {
            md.index.push_back(f);
            md.value.push_back(1.0);
        }
        md.index.push_back(ins.n);  // runway-count row
        md.value.push_back(1.0);
    }
    md.start.push_back(static_cast<HighsInt>(md.index.size()));
    md.lower.assign(nc, 0.0);
    md.upper.assign(nc, 1.0);
    md.integrality.assign(nc, kHighsVarTypeInteger);
    md.rowLower.assign(ins.n + 1, 1.0);
    md.rowUpper.assign(ins.n + 1, 1.0);
    md.rowLower[ins.n] = 0.0;
    md.rowUpper[ins.n] = ins.m;
    return md;
}

bool isPartition(const Instance& ins, const ColumnPool& pool, const std::vector<int>& chosen) {
    if (static_cast<int>(chosen.size()) > ins.m) return false;
    std::vector<int> seen(ins.n, 0);
    for (int id : chosen)
        for (int f : pool.col(id).seq) ++seen[f];
    return std::all_of(seen.begin(), seen.end(), [](int v) { return v == 1; });
}

}  // namespace

SpResult solveSetPartitioning(const Instance& ins, const ColumnPool& pool,
                              const std::vector<int>& colIds, const std::vector<int>& warm,
                              double timeLimit, bool verbose) {
    SpResult res;
    const double t0 = nowSec();
    void* h = Highs_create();
    if (Highs_getSizeofHighsInt(h) != static_cast<HighsInt>(sizeof(HighsInt))) {
        std::fprintf(stderr, "HiGHS HighsInt size mismatch\n");
        Highs_destroy(h);
        return res;
    }
    Highs_setBoolOptionValue(h, "output_flag", verbose ? 1 : 0);
    Highs_setIntOptionValue(h, "threads", 1);
    Highs_setStringOptionValue(h, "presolve", "off");  // presolve is slow on long columns and ignores time_limit
    Highs_setDoubleOptionValue(h, "time_limit", std::max(0.1, timeLimit));
    Highs_setDoubleOptionValue(h, "mip_rel_gap", 0.0);
    Highs_setDoubleOptionValue(h, "mip_abs_gap", 0.999);  // integer costs

    const Model md = buildModel(ins, pool, colIds);
    const HighsInt nc = static_cast<HighsInt>(colIds.size());
    const HighsInt st = Highs_passMip(h, nc, ins.n + 1, static_cast<HighsInt>(md.index.size()),
                                      kHighsMatrixFormatColwise, kHighsObjSenseMinimize, 0.0,
                                      md.cost.data(), md.lower.data(), md.upper.data(),
                                      md.rowLower.data(), md.rowUpper.data(), md.start.data(),
                                      md.index.data(), md.value.data(), md.integrality.data());
    if (st < 0) {
        std::fprintf(stderr, "HiGHS passMip failed\n");
        Highs_destroy(h);
        return res;
    }
    if (!warm.empty()) {
        std::unordered_set<int> w(warm.begin(), warm.end());
        std::vector<double> x0(nc, 0.0);
        for (HighsInt j = 0; j < nc; ++j) x0[j] = w.count(colIds[j]) ? 1.0 : 0.0;
        Highs_setSolution(h, x0.data(), nullptr, nullptr, nullptr);
    }
    Highs_run(h);
    res.status = Highs_getModelStatus(h);
    HighsInt pss = 0;
    Highs_getIntInfoValue(h, "primal_solution_status", &pss);
    Highs_getDoubleInfoValue(h, "mip_dual_bound", &res.lpBound);
    if (pss == 2) {
        std::vector<double> x(nc), cd(nc), rv(ins.n + 1), rd(ins.n + 1);
        Highs_getSolution(h, x.data(), cd.data(), rv.data(), rd.data());
        for (HighsInt j = 0; j < nc; ++j)
            if (x[j] > 0.5) res.chosen.push_back(colIds[j]);
        if (isPartition(ins, pool, res.chosen)) {
            res.feasible = true;
            res.optimal = (res.status == kHighsModelStatusOptimal);
            for (int id : res.chosen) res.cost += pool.col(id).cost;
        }
    }
    Highs_destroy(h);
    res.seconds = nowSec() - t0;
    return res;
}
