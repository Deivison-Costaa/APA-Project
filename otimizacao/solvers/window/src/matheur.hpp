// Window sweeps (POPMUSIC-like): slide a time window over the horizon and solve each
// subproblem exactly with WindowOpt, polishing with the LS after every improvement.
#pragma once
#include "ls.hpp"
#include "timer.hpp"
#include "window.hpp"

struct SweepConfig {
    int wSize = 30;        // target number of free flights per window
    int wRunways = 0;      // runways per window (0 = all)
    double mipTimeLimit = 2.0;
    int maxFree = 80;
    int verbose = 1;
};

// One sweep over the whole horizon (random offset). Returns total delta.
long long windowSweep(Solution& s, WindowOpt& wo, LocalSearch& ls, Rng& rng, const SweepConfig& cfg,
                      WindowStats& stats, const Timer& timer, double deadline);
