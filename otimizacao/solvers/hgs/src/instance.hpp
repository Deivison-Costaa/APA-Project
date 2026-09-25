// Instance data and whole-solution utilities for P | r_j, s_ij | sum p_j (S_j - r_j).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

using Routes = std::vector<std::vector<int>>;

// Setup times are stored compactly (the n x n matrix must stay in cache). The Copa
// instances have t <= 50; build with -DSETUP_T=int32_t for instances with larger values.
#ifndef SETUP_T
#define SETUP_T uint8_t
#endif
using SetupT = SETUP_T;

struct Instance {
    int n = 0, m = 0;
    std::vector<int> r, c, p;
    std::vector<SetupT> t;   // n*n, row = from
    int horizon = 0;         // max release + max(c) + max(t)
    std::string name;        // file basename without extension

    int T(int i, int j) const { return t[(size_t)i * n + j]; }
};

// Throws std::runtime_error on malformed input.
Instance readInstance(const std::string& path);

// Full evaluation of a set of runways; fills start times (size n) when S != nullptr.
long long evaluateRoutes(const Instance& ins, const Routes& routes, std::vector<int>* S = nullptr);

// Validates that every flight appears exactly once; returns an empty string when valid.
std::string validateRoutes(const Instance& ins, const Routes& routes);

// Reads a solution file (first line cost, then m lines of 1-indexed flights).
Routes readSolution(const Instance& ins, const std::string& path);

// Writes a solution file atomically (tmp + rename). Returns false on I/O failure.
bool writeSolution(const Instance& ins, const Routes& routes, long long cost, const std::string& path);
