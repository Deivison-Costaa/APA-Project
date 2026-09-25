// Hybrid Genetic Search driver (one island per thread, best shared across islands).
#pragma once
#include <atomic>
#include <chrono>
#include <climits>
#include <mutex>
#include <string>

#include "instance.hpp"
#include "params.hpp"

using Clock = std::chrono::steady_clock;

class SharedBest {
public:
    SharedBest(const Instance& ins, std::string outPath, Clock::time_point t0)
        : I(ins), out_(std::move(outPath)), t0_(t0) {}
    // Records a solution if it improves the global best; writes it to disk (throttled).
    bool offer(const Routes& routes, long long cost, int thread);
    long long cost() const { return cost_.load(); }
    Routes routes();
    void flush();  // writes the best solution (if not yet written)
    double elapsed() const { return std::chrono::duration<double>(Clock::now() - t0_).count(); }

private:
    const Instance& I;
    std::string out_;
    Clock::time_point t0_;
    std::mutex mu_;
    std::atomic<long long> cost_{LLONG_MAX};
    Routes routes_;
    bool dirty_ = false;
    double lastWrite_ = -1e9;
};

struct GeneticStats {
    long long generations = 0, restarts = 0, lsMoves = 0, lsEvals = 0;
    long long bestCost = LLONG_MAX;
    long long kindCount[10] = {}, kindBetterParents[10] = {}, kindNewBest[10] = {};
};

GeneticStats runGenetic(const Instance& ins, const Params& par, uint64_t seed, int threadId, double deadline,
                        SharedBest& shared);
