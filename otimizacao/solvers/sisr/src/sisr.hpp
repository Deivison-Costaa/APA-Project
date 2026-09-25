// SISR (Slack Induction by String Removals) ruin & recreate inside simulated annealing.
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

#include "instance.hpp"
#include "rng.hpp"
#include "solution.hpp"

struct Params {
    double timeLimit = 60.0;
    uint64_t seed = 1;
    double cbar = 10.0;       // average number of removed flights
    int lmax = 10;            // max string length
    double splitProb = 0.5;   // probability of split-string removal
    double keepProb = 0.5;    // growth probability of the preserved block
    double blink = 0.01;      // probability of skipping a position in recreate
    int window = 60;          // insertion positions start where S >= r_j - window
    double T0 = 100.0;         // initial temperature
    double Tf = 10.0;          // final temperature
    int adjK = 80;            // time neighbours per flight
    double tailProb = 0.3;    // probability of swapping tails of two ruined runways
    double seedBias = 0.5;    // probability of biasing the seed towards delayed flights
    double sliceProb = 0.2;   // probability of a time-slice ruin instead of strings
    int sliceW = 100;          // max half width of the slice
    int wRandom = 4, wRAsc = 4, wPDesc = 2, wRDesc = 1;  // recreate order weights
    double globalFrac = 0.1;  // fraction of the time spent in global SA (rest: windowed SA)
    int wHalf = 200;          // window half width drawn from [wHalf/2, 1.5 wHalf)
    long long wIters = 100000;  // SA iterations per window
    double wT0 = 100.0, wTf = 5.0;  // window SA temperatures
    double wSpread = 1.0;     // per-window wT0 and wIters scaled by log-uniform [1/s, s]
    int wFixLo = 0, wFixHi = 0;  // debug: fixed seed window [lo, hi] in time
    double hotProb = 0.7;     // probability that the window covers a delay cluster
    int clusterGap = 60;      // delayed flights closer than this (in start time) share a cluster
    double clusterPow = 1.0;  // cluster pick probability ~ cost^clusterPow
    int marginMin = 20, marginMax = 120;  // random margin added around the cluster
    double optProb = 0.5;     // probability of a best 2-opt* (tail exchange) move per iteration
    double reportEvery = 5.0;
    int verbose = 1;
    bool selfCheck = false;   // verify incremental cost every iteration
};

// Global best shared between threads.
struct SharedBest {
    std::mutex mtx;
    std::atomic<long long> cost{kInf};
    Seqs seqs;
    bool offer(long long c, const Seqs& s) {
        if (c >= cost.load(std::memory_order_relaxed)) return false;
        std::lock_guard<std::mutex> lk(mtx);
        if (c >= cost.load()) return false;
        cost.store(c);
        seqs = s;
        return true;
    }
};

struct RunStats {
    long long iters = 0;
    long long accepted = 0;
    long long improvements = 0;
    long long windows = 0;
    long long tailMoves = 0;
    double seconds = 0;
};

class SisrSolver {
public:
    SisrSolver(const Instance& I, const Params& P, int threadId);

    void setInitial(const Seqs& seqs);   // warm start
    void construct();                    // greedy insertion by release time
    // Runs SA until the time limit (seconds measured from `startTime`).
    RunStats run(SharedBest& shared, double startTime);

    long long bestCost() const { return bestCost_; }
    const Seqs& bestSeqs() const { return best_; }

private:
    void buildAdjacency();
    void step(double T, RunStats& st);
    void stepTail(double T, RunStats& st);
    void report(double elapsed, double T, const RunStats& st, const char* phase);
    void setSeedWindow(int tLo, int tHi);
    void pickWindow(int& tLo, int& tHi);
    void ruin();
    int pickSeed();
    void ruinStrings(int seed);
    void ruinSlice(int seed);
    void removeRange(int k, int a, int b);
    void recreate(bool allowBlink);
    void touch(int k);
    void swapTails(int ka, int ca, int kb, int cb);
    void restore();
    void commit();
    void orderRemoved();
    bool verify() const;

    const Instance& I_;
    Params P_;
    int tid_;
    Rng rng_;
    Solution cur_;
    Seqs best_;
    long long bestCost_ = kInf;

    std::vector<std::vector<int>> adj_;
    std::vector<int> removed_;
    std::vector<std::pair<int, int>> cuts_;  // (runway, gap index) of this ruin
    std::vector<int> tmp_, perm_;
    std::vector<char> touched_;
    std::vector<int> touchedList_;
    std::vector<Runway> saved_;
    uint32_t blinkThresh_ = 0;
    std::vector<int> byR_;  // flights sorted by release
    int seedLo_ = 0, seedHi_ = 0;  // seeds are drawn from byR_[seedLo_, seedHi_)
    struct Cluster {
        int tLo, tHi;
        double weight;
    };
    std::vector<std::pair<long long, int>> hot_;
    std::vector<Cluster> clusters_;
    SharedBest* shared_ = nullptr;
    double lastReport_ = 0.0;
};

double nowSeconds();
