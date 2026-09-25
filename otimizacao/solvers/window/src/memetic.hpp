// Multi-epoch search: each epoch runs an annealed ILS from a fresh (or given) start, then the
// result is recombined with an elite archive by exact time-cut crossover; children are polished
// with the LS and an exact window MIP around the cut. Optional window sweeps on epoch results.
#pragma once
#include <string>
#include <vector>

#include "matheur.hpp"
#include "search.hpp"

struct MemeticConfig {
    int epochs = 3;            // number of ILS epochs sharing the time budget
    int archiveSize = 8;
    bool sweepEpochs = true;   // window sweep on each epoch result
    double sweepFrac = 0.08;   // max share of an epoch's time spent in its sweep
    int cutWindow = 1;         // exact window around the crossover cut (0 = off)
    IlsConfig ils;
    SweepConfig sweep;
    std::string tag;
    int verbose = 1;
};

struct MemeticStats {
    long long crossovers = 0, childImproved = 0, childBest = 0;
};

Solution runMemetic(const Instance& I, Solution first, bool firstIsInit, LocalSearch& ls, WindowOpt& wo,
                    WindowStats& wst, Rng& rng, const MemeticConfig& cfg, const Timer& timer, double deadline,
                    const BestCallback& onBest, MemeticStats& mst);
