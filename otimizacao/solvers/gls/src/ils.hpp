// Iterated local search with simulated-annealing-like acceptance and
// multi-thread independent chains sharing the best solution.
#pragma once
#include <mutex>
#include <string>
#include <vector>

#include "localsearch.hpp"
#include "neighbors.hpp"
#include "perturb.hpp"

struct ILSParams {
    double timeLimit = 60.0;
    long long maxIter = -1;   // optional iteration budget (deterministic comparisons)
    uint64_t seed = 1;
    int threads = 1;
    double T0 = 300.0;        // initial temperature (cost units)
    double Tf = 5.0;         // final temperature
    double syncPeriod = 5.0; // seconds between best-sharing among threads
    double cycle = 100.0;    // >0: annealing restarts (from the best) every ~cycle seconds
    double reportEvery = 5.0;
    double resetFactor = 0.0;  // if >0: reload best when cur > best*(1+resetFactor)
    std::string outPath;       // file written with the best solution (periodically + at end)
    LSParams ls;
    PerturbParams pert;
    bool verbose = true;
    bool debugCheck = false;   // verify incremental cost against reference after every iteration
    int assignMode = 1;        // 0 off, 1 tail+segment assignment around the perturbation after LS
};

struct SharedBest {
    std::mutex mu;
    ll cost = INF_COST;
    std::vector<std::vector<int>> seqs;
    double lastWrite = -1e9;
    ll written = INF_COST;
};

std::vector<std::vector<int>> greedyConstruct(const Instance& in);

// Runs the ILS from `init`; result stored in `best`.
void runILS(const Instance& in, const Neighbors& nb, const ILSParams& prm,
            const std::vector<std::vector<int>>& init, SharedBest& best);
