// Thin wrapper around HiGHS for a 0/1 model given column-wise.
#pragma once
#include <vector>

struct BinModel {
    int numRows = 0;
    std::vector<double> cost;           // one per column
    std::vector<int> start, index;      // column-wise sparse matrix (start has numCols+1 entries)
    std::vector<double> value;
    std::vector<double> rowLo, rowUp;
    int numCols() const { return static_cast<int>(cost.size()); }
};

struct MipResult {
    bool feasible = false;
    bool optimal = false;
    double objective = 0;
    double bound = 0;
    double rootBound = 0;
    long long nodes = 0;
    std::vector<double> x;
};

// Solve min cost.x s.t. rowLo <= A x <= rowUp, x in {0,1}. `init` (optional) is a feasible
// starting solution. Single-threaded, time-limited.
MipResult solveBinary(const BinModel& m, const std::vector<double>* init, double timeLimit);
