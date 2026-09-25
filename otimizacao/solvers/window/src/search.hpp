// Construction, ruin-and-recreate perturbation and the ILS driver.
#pragma once
#include <functional>
#include <string>

#include "ls.hpp"
#include "solution.hpp"
#include "timer.hpp"

Solution greedyConstruct(const Instance& I);

// Best insertion of flight x into the current solution (positions near r_x on each runway).
void insertBest(Solution& s, int x, Rng& rng, LocalSearch* ls);

// Remove flights `rem` from the solution (they must be present).
void removeFlights(Solution& s, const std::vector<int>& rem);

// Ruin & recreate around a random flight. Returns removed flights.
// focus = probability of seeding the ruin at a delayed flight (instead of a uniform one).
std::vector<int> ruinRecreate(Solution& s, LocalSearch& ls, Rng& rng, int kRemove, double focus = 0.0);

struct IlsConfig {
    double timeLimit = 60;
    int kMin = 4, kMax = 14;
    double temp0 = 0.0, temp1 = 0.0;  // 0 = auto
    double focus = 0.0;
    int verbose = 1;
    std::string tag;
};

using BestCallback = std::function<void(const Solution&)>;

// Iterated local search with SA acceptance on local optima. Returns the best solution.
Solution runIls(const Instance& I, Solution start, LocalSearch& ls, Rng& rng, const IlsConfig& cfg,
                const Timer& timer, double deadline, const BestCallback& onBest);
