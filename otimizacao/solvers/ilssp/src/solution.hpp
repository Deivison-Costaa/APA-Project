// Solution representation: m runway sequences with cached start times and
// prefix costs, plus per-flight position lookups.
#pragma once

#include <string>
#include <vector>

#include "instance.hpp"

struct Runway {
    std::vector<int> seq;        // flights in order
    std::vector<int> S;          // start time per position
    std::vector<long long> pc;   // pc[q] = cost of positions [0, q); size = len + 1

    int size() const { return static_cast<int>(seq.size()); }
    long long cost() const { return pc.back(); }
};

class Solution {
public:
    Solution() = default;
    explicit Solution(const Instance& ins);

    std::vector<Runway> rw;
    std::vector<int> rwOf;     // runway of each flight
    std::vector<int> posOf;    // position of each flight in its runway
    std::vector<int> startOf;  // start time of each flight
    long long cost = 0;

    // Recomputes schedule of runway k from position `from` on. Returns the
    // last position whose start time changed or that lies before `structEnd`
    // (i.e. the end of the disturbed region), or from-1 if none.
    int recompute(const Instance& ins, int k, int from, int structEnd);
    void recomputeAll(const Instance& ins);
    std::vector<std::vector<int>> sequences() const;
};

long long sequenceCost(const Instance& ins, const std::vector<int>& seq);
Solution solutionFromRunways(const Instance& ins, const std::vector<std::vector<int>>& runways);

// Full independent check: each flight exactly once and cached data consistent.
bool validateSolution(const Instance& ins, const Solution& s, std::string& err);

// Reads a solution file (cost line + up to m runway lines, 1-indexed flights).
bool readSolutionFile(const std::string& path, const Instance& ins,
                      std::vector<std::vector<int>>& runways, long long& claimed,
                      std::string& err);

// Writes atomically (tmp file + rename). Returns false on I/O error.
bool writeSolutionFile(const std::string& path, const Solution& s);
