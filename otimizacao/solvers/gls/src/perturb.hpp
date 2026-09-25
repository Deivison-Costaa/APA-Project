// Time-localized ruin & recreate perturbation.
#pragma once
#include <vector>

#include "common.hpp"
#include "neighbors.hpp"
#include "state.hpp"

struct PerturbParams {
    int kmin = 4;           // flights removed (min)
    int kmax = 16;          // flights removed (max)
    double pWindow = 0.5;   // probability of time-window ruin (else strings from a few runways)
    int maxRunways = 5;     // string ruin: max number of runways hit
    int insWin = 2;         // insertion slots tried around the time index in each runway
    double blink = 0.02;    // probability of skipping an insertion slot
    double pDelayedSeed = 0.5;  // probability that the seed flight is a delayed one
};

class Perturber {
public:
    Perturber(State& st, const Neighbors& nb, const PerturbParams& prm);
    void ruinRecreate(Rng& rng);
    // greedy best insertion of flight f (not currently in any runway)
    void insertBest(int f, Rng& rng, double blink);
    int lastCenter() const { return center_; }

private:
    int pickSeed(Rng& rng);
    void ruinWindow(int f0, int k, Rng& rng);
    void ruinStrings(int f0, int k, Rng& rng);
    void removeMarked();
    void orderRemoved(Rng& rng);

    State& st_;
    const Neighbors& nb_;
    PerturbParams prm_;
    std::vector<int> removed_;
    std::vector<char> mark_;
    std::vector<int> buf_;
    std::vector<int> runwayMark_;
    std::vector<double> key_;
    int center_ = 0;
};
