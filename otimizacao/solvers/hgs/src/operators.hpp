// Recombination / construction operators. Each produces a partial set of runways plus
// the list of flights left out; the local search inserts those and educates the child.
#pragma once
#include <algorithm>
#include <vector>

#include "instance.hpp"
#include "params.hpp"
#include "population.hpp"

struct Offspring {
    Routes routes;
    std::vector<int> missing;
    int kind = 0;       // statistics bucket (operator and window size class)
};

// Statistics buckets: 0 = construction, 1..4 = crossover window size class,
// 5..8 = ruin window size class, 9 = runway crossover.
constexpr int kNumKinds = 10;
inline int sizeClass(double frac, double lo, double hi) {
    double x = (frac - lo) / (hi - lo + 1e-12);
    return std::min(3, std::max(0, (int)(x * 4)));
}

// Time-window crossover: A's runways outside [T1, T2), B's runways inside, with the
// runway pieces joined by two min-cost assignments (Hungarian, m x m).
Offspring crossoverTimeWindow(const Instance& ins, const Params& par, const Individual& A, const Individual& B,
                              Rng& rng);

// Runway-exchange crossover (SREX-like): k whole runways of A, the m-k runways of B that
// lose the fewest flights to them (duplicates removed); the rest is reinserted.
Offspring crossoverRunways(const Instance& ins, const Individual& A, const Individual& B, Rng& rng);

// Ruin & recreate mutation: drop the flights of a random time window on a random subset of
// runways of A (they are reinserted by cheapest insertion).
Offspring ruinWindow(const Instance& ins, const Params& par, const Individual& A, Rng& rng);

// Randomised greedy construction (release order + noise, best runway to append to).
Routes randomGreedy(const Instance& ins, Rng& rng);

// Min-cost perfect assignment (square matrix). Returns col assigned to each row.
std::vector<int> hungarian(const std::vector<std::vector<long long>>& cost);
