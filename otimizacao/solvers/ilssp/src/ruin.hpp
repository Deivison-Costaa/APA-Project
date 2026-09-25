// Ruin & recreate perturbation: removes short strings of consecutive flights
// from a few runways around a seed time, then greedily reinserts them at the
// cheapest position (with "blinks") near their release times.
#pragma once

#include <vector>

#include "editor.hpp"
#include "eval.hpp"
#include "rng.hpp"

struct RuinParams {
    int maxRunways = 4;       // runways affected per ruin
    int maxString = 4;        // max string length removed per runway
    double blink = 0.03;      // probability of skipping a candidate position
    double delayedSeed = 0.5; // probability of seeding at a delayed flight
    int insertWindow = 2;     // positions q-w..q+w tried per runway
};

class RuinRecreate {
public:
    RuinRecreate(const Instance& ins, Rng& rng, RuinParams prm);

    // Perturbs sol (through editor, so the journal sees every touched runway)
    // and appends changed regions to `regions`.
    void perturb(Editor& editor, std::vector<Region>& regions);

    RuinParams& params() { return prm_; }

private:
    const Instance& ins_;
    Rng& rng_;
    RuinParams prm_;
    Evaluator ev_;
    std::vector<int> removed_;
    std::vector<int> delayed_;
    std::vector<int> runwayOrder_;

    int pickSeed(const Solution& s);
    void insertBest(Editor& editor, int x, std::vector<Region>& regions);
};
