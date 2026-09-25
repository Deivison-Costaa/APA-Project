// Set partitioning over pool columns, solved with HiGHS:
//   min sum c_k x_k  s.t.  every flight covered exactly once, sum x_k <= m.
#pragma once

#include <vector>

#include "colpool.hpp"
#include "solution.hpp"

struct SpResult {
    bool feasible = false;       // a valid partition was returned
    bool optimal = false;        // proven optimal over the given columns
    long long cost = 0;
    std::vector<int> chosen;     // column ids (pool ids)
    double seconds = 0.0;
    double lpBound = 0.0;        // HiGHS dual bound
    int status = 0;
};

// `colIds` are pool ids to include; `warm` are pool ids of a feasible
// partition (subset of colIds) used as MIP start.
SpResult solveSetPartitioning(const Instance& ins, const ColumnPool& pool,
                              const std::vector<int>& colIds, const std::vector<int>& warm,
                              double timeLimit, bool verbose);
