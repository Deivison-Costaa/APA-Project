// Time-cut crossover: child = prefix of A (flights starting before T) + suffix chains of B
// (flights starting at/after T), re-attached to A's runway ends by an exact m x m assignment.
// Only "clean" cuts are used: every flight lies on the same side of T in A and in B.
#pragma once
#include "solution.hpp"

struct CutResult {
    bool found = false;
    long long cost = 0;
    int T = 0;
};

// Scans all clean cuts; if the best child is cheaper than `limit`, builds it into `child`.
CutResult bestCutChild(const Solution& A, const Solution& B, long long limit, Solution& child);
