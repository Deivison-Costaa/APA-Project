// Instance data for P | r_j, s_ij | sum p_j (S_j - r_j).
#pragma once
#include <string>
#include <vector>

struct Instance {
    int n = 0;                 // flights
    int m = 0;                 // runways
    std::vector<int> r, c, p;  // release, occupation, penalty
    std::vector<int> t;        // n*n, row = from
    std::vector<int> tminIn;   // min_i t[i][j] (lower bound on any setup into j)
    std::string name;          // basename without extension

    int T(int i, int j) const { return t[static_cast<size_t>(i) * n + j]; }
};

// Throws std::runtime_error on malformed input.
Instance readInstance(const std::string& path);
