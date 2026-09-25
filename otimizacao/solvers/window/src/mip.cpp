#include "mip.hpp"

#include "Highs.h"

MipResult solveBinary(const BinModel& m, const std::vector<double>* init, double timeLimit) {
    MipResult res;
    const int nc = m.numCols();
    Highs highs;
    highs.setOptionValue("output_flag", false);
    highs.setOptionValue("threads", 1);
    highs.setOptionValue("time_limit", timeLimit);
    highs.setOptionValue("mip_rel_gap", 0.0);
    highs.setOptionValue("mip_abs_gap", 0.999);  // objective is integral
    std::vector<double> lo(nc, 0.0), up(nc, 1.0);
    std::vector<HighsInt> integ(nc, 1);
    std::vector<HighsInt> st(m.start.begin(), m.start.end()), idx(m.index.begin(), m.index.end());
    const HighsStatus ps = highs.passModel(nc, m.numRows, static_cast<HighsInt>(idx.size()),
                                           static_cast<HighsInt>(MatrixFormat::kColwise),
                                           static_cast<HighsInt>(ObjSense::kMinimize), 0.0, m.cost.data(),
                                           lo.data(), up.data(), m.rowLo.data(), m.rowUp.data(), st.data(),
                                           idx.data(), m.value.data(), integ.data());
    if (ps == HighsStatus::kError) return res;
    if (init) {
        HighsSolution sol;
        sol.col_value = *init;
        sol.value_valid = true;
        highs.setSolution(sol);
    }
    if (highs.run() == HighsStatus::kError) return res;
    const HighsModelStatus ms = highs.getModelStatus();
    const HighsInfo& info = highs.getInfo();
    res.optimal = (ms == HighsModelStatus::kOptimal);
    res.feasible = info.primal_solution_status == kSolutionStatusFeasible;
    if (res.feasible) {
        res.x = highs.getSolution().col_value;
        res.objective = info.objective_function_value;
        res.bound = info.mip_dual_bound;
        res.nodes = info.mip_node_count;
    }
    return res;
}
