// Min-cost perfect assignment (Hungarian, O(N^3)) with dual potentials.
#pragma once
#include <vector>

struct AssignResult {
    long long cost = 0;
    std::vector<int> colOfRow;      // assignment
    std::vector<long long> u, v;    // duals: u[i] + v[j] <= a[i][j], equality on assigned pairs
};

// a is N x N row-major; use a large value (e.g. 1e13) for forbidden pairs.
AssignResult hungarian(const std::vector<long long>& a, int N);
