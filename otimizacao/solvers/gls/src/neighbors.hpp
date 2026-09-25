// Granular candidate lists: for each flight, its most promising predecessors and successors.
#pragma once
#include <vector>

#include "instance.hpp"

struct Neighbors {
    int K = 0;
    std::vector<std::vector<int>> pred;  // pred[f]: flights g that are good immediately before f
    std::vector<std::vector<int>> succ;  // succ[f]: flights h that are good immediately after f
    std::vector<int> byRelease;          // flights sorted by release time
    std::vector<int> rankOf;             // position of each flight in byRelease
};

// score(i -> j) = p_j * max(0, r_i + c_i + t_ij - r_j) + idleWeight * max(0, r_j - r_i - c_i - t_ij)
Neighbors buildNeighbors(const Instance& in, int K, double idleWeight);
