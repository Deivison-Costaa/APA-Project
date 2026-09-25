// Iterated local search with ruin & recreate perturbation and
// simulated-annealing acceptance. Feeds runways of good local optima to a
// column pool for the set-partitioning recombination.
#pragma once

#include <atomic>
#include <functional>

#include "colpool.hpp"
#include "localsearch.hpp"
#include "ruin.hpp"

struct IlsParams {
    double T0 = 1.0;        // start temperature, in units of the mean penalty
    double Tf = 0.05;       // final temperature, same units
    double poolTol = 0.002;  // local optima within best*(1+tol) feed the pool
    bool selfTest = false;  // verify deltas and caches (slow)
    const std::atomic<bool>* stop = nullptr;  // raised by the driver when --target is reached
};

struct IlsStats {
    long long iterations = 0;
    long long accepted = 0;
    long long lsMoves = 0;
    long long newColumns = 0;
};

class Ils {
public:
    Ils(const Instance& ins, uint64_t seed, IlsParams ip, LsParams lp, RuinParams rp);

    // Runs from `cur` for `seconds`. `best` is updated whenever improved and
    // onBest is called with it. Columns go to `pool` when not null.
    void run(Solution& cur, Solution& best, double seconds, ColumnPool* pool, long long stamp,
             IlsStats& st, const std::function<void(const Solution&)>& onBest);

    // Plain local search to a local optimum (no journal).
    void descend(Solution& s);

    Rng& rng() { return rng_; }
    IlsParams& params() { return ip_; }
    RuinParams& ruinParams() { return rr_.params(); }
    long long evaluations() const { return ls_.evaluations(); }

private:
    const Instance& ins_;
    Rng rng_;
    IlsParams ip_;
    LocalSearch ls_;
    RuinRecreate rr_;
    Journal journal_;
    std::vector<Region> regions_;
    double meanP_ = 1.0;
};
