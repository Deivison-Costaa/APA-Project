// Time-window subproblem: flights whose start lies in [T0,T1) on a set of runways are freed;
// prefixes stay fixed, post-window suffix chains may be re-attached to any runway of the set.
// Solved exactly (within a delay cap) as an arc-time-indexed 0/1 flow model with HiGHS.
#pragma once
#include <vector>

#include "solution.hpp"

struct WindowStats {
    long long calls = 0, improved = 0, optimal = 0, timeouts = 0, skipped = 0, provenByLb = 0;
    long long gain = 0;
    long long sumFree = 0, sumStates = 0, sumArcs = 0;
    double mipTime = 0, lbTime = 0;
    int verbose = 0;
};

class WindowOpt {
public:
    WindowOpt(const Instance& inst, int delayCap) : I(inst), dcap_(delayCap) {}
    // Optimizes the window in place. Returns the (non-positive) cost change.
    // `touched` receives the flights whose runway neighbourhood changed.
    long long optimize(Solution& s, int T0, int T1, const std::vector<int>& runways, double timeLimit,
                       WindowStats& st, std::vector<int>* touched, int maxFree = 1 << 30);

private:
    const Instance& I;
    int dcap_;
};
