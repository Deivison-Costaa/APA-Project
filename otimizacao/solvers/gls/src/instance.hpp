// Problem instance: P | r_j, s_ij | sum p_j (S_j - r_j).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "common.hpp"

struct Instance {
    int n = 0;
    int m = 0;
    std::vector<int> r;  // release time
    std::vector<int> c;  // runway occupation
    std::vector<int> p;  // penalty per unit of delay
    // ct[i*n + j] = c_i + t_ij  (minimum distance between start of i and start of j when j follows i)
    std::vector<uint16_t> ct;
    std::vector<uint8_t> ct8;  // same table in 8 bits when all values fit (better cache use)
    bool small = false;
    std::string name;  // base name without extension

    int gap(int i, int j) const {
        const size_t k = static_cast<size_t>(i) * n + j;
        return small ? ct8[k] : ct[k];
    }
};

// Throws std::runtime_error with a clear message on malformed input.
Instance loadInstance(const std::string& path);

// Solution file I/O (1-indexed flights, first line cost). Throws on malformed input.
std::vector<std::vector<int>> readSolutionFile(const Instance& in, const std::string& path);
void writeSolutionFile(const std::string& path, ll cost, const std::vector<std::vector<int>>& seqs);

// Reference cost computation (straightforward, used for self-checks).
ll referenceCost(const Instance& in, const std::vector<std::vector<int>>& seqs);
