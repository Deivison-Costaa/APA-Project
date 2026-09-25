// Adaptive neighborhood width. The granular moves only try positions near a
// flight's release time, which fits instances where few flights are delayed
// (Copa APA: 4-16%). On congested instances most flights start far from r_j and
// the narrow windows miss the right positions, so wider ones are used there.
#pragma once

#include "options.hpp"
#include "solution.hpp"

struct AdaptResult {
    double delayedFrac = 0.0;  // delayed flights after a first local search
    bool wide = false;         // wide windows chosen
    bool applied = false;      // false when the user fixed the windows or --no-adapt
};

// Runs a local search from `start` (on a copy), measures congestion and, unless
// the windows were set on the command line, widens them in `opt` when needed.
AdaptResult adaptWindows(const Instance& ins, const Solution& start, Options& opt);
