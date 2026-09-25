// Tunable parameters and a small fast RNG.
#pragma once
#include <cstdint>
#include <string>

struct Params {
    // run control
    double timeLimit = 60.0;
    uint64_t seed = 1;
    int threads = 1;
    std::string initFile;
    std::string outPath;
    int verbose = 1;

    // local search
    int nbCorr = 20;      // neighbours by fit score (predecessor/successor slack)
    int nbTime = 10;      // neighbours by release-time proximity
    int maxIntraWin = 14; // max window for intra-runway moves
    bool focused = true;  // children: start the descent around modified flights only
    bool ejection = false;// depth-2 ejection chains (u replaces w, w moves to a third runway)

    // genetic search (Vidal-style HGS)
    int mu = 25;
    int lambda = 40;
    int nbElite = 4;
    int nbClose = 5;
    int itNoImprove = 4000;  // generations without improvement before restart
    int initPop = 50;         // initial random individuals (upper bound; time capped)
    double initFrac = 0.05;   // max fraction of the time limit spent building a population
    bool keepBest = false;    // re-inject the global best after every restart
    int eliteEvery = 3;       // every k-th restart starts from the archive of run bests (0 = never)
    int archiveSize = 20;     // number of archived run bests
    std::string xover = "mix"; // tw | rx | mix | none
    double winMin = 0.10, winMax = 0.60;  // time-window crossover length (fraction of horizon)
    double ruinMin = 0.01, ruinMax = 0.08; // ruin window length (fraction of horizon)
    double ruinTargeted = 0.7;             // prob. of centring the ruin window on a delayed flight
    double ruinRouteProb = 0.5;            // prob. that a runway is hit by the ruin window
    double pTw = 0.7;                      // mix: probability of crossover (else ruin)
    double twTargeted = 0.0;               // prob. of centring the crossover window on a difference
    double pSortedInsert = 0.5;            // prob. of reinserting missing flights in release order
    std::string dist = "starts";           // arcs | starts | mix  (diversity distance)
};

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed * 0x9E3779B97F4A7C15ULL + 0x1234567ULL) { next(); }
    uint64_t next() {
        s += 0x9E3779B97F4A7C15ULL;
        uint64_t z = s;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    int uniform(int n) { return (int)((next() >> 33) % (uint64_t)n); }  // [0, n)
    double real() { return (next() >> 11) * (1.0 / 9007199254740992.0); }
    // UniformRandomBitGenerator interface for std::shuffle
    using result_type = uint64_t;
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return ~0ULL; }
    uint64_t operator()() { return next(); }
};
