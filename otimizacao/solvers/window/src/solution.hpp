// Solution representation + incremental (sync-stop) runway evaluation.
#pragma once
#include <string>
#include <vector>

#include "instance.hpp"

struct Solution {
    const Instance* I = nullptr;
    std::vector<std::vector<int>> seq;        // flights per runway
    std::vector<std::vector<int>> st;         // start time per position
    std::vector<std::vector<long long>> cum;  // cum[k][q] = cost of first q flights (size len+1)
    std::vector<int> rwOf, posOf;             // runway / position of each flight
    long long cost = 0;

    Solution() = default;
    explicit Solution(const Instance& inst);

    int len(int k) const { return static_cast<int>(seq[k].size()); }
    long long rwCost(int k) const { return cum[k].back(); }

    // Recompute times/costs of runway k from position `from` on; updates rwOf/posOf and total cost.
    void recompute(int k, int from = 0);
    void recomputeAll();
    // Replace runway k by a new sequence; `firstDiff` = first position that may differ.
    void setRunway(int k, std::vector<int> newSeq, int firstDiff = 0);

    // Cost of a runway made of: prefix of runway P ([0,a)), then X[0..nx), then the suffix of
    // runway Q from position b (original times used for the sync-stop). P or Q may be -1.
    long long evalConcat(int P, int a, const int* X, int nx, int Q, int b) const;

    long long fullCost() const;  // recompute from scratch (no caches)
    bool valid(std::string* why = nullptr) const;
};

// IO
Solution readSolution(const Instance& I, const std::string& path);  // throws on error
void writeSolution(const Solution& s, const std::string& path);      // atomic (tmp + rename)
