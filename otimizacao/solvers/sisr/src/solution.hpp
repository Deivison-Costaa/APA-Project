// Full solution: m runways plus flight -> runway index, and solution file IO.
#pragma once
#include <string>
#include <vector>

#include "instance.hpp"
#include "runway.hpp"

using Seqs = std::vector<std::vector<int>>;

struct Solution {
    std::vector<Runway> rw;
    std::vector<int> rwOf;  // runway of each flight, -1 if unassigned
    long long cost = 0;

    void init(const Instance& I, const Seqs& seqs);  // seqs are 0-indexed
    Seqs seqs() const;
    long long recomputeCost(const Instance& I) const;  // from scratch, no mutation
};

// Independent cost evaluation of 0-indexed sequences (mirrors check.py).
long long evaluate(const Instance& I, const Seqs& seqs);

// Throws std::runtime_error if the file is invalid for the instance.
Seqs readSolutionFile(const Instance& I, const std::string& path);

// Writes atomically (tmp + rename). Returns false on IO error.
bool writeSolutionFile(const Instance& I, const Seqs& seqs, long long cost, const std::string& path);
