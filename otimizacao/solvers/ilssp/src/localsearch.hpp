// Granular local search: relocate, swap, 2-opt*, or-opt between runways and
// small intra-runway reorderings. Candidate positions in a target runway are
// taken around the flight's release time. A per-(flight, runway) bitmask of
// "dirty" pairs (don't-look bits) limits re-evaluation to changed regions.
#pragma once

#include <cstdint>
#include <vector>

#include "editor.hpp"
#include "eval.hpp"
#include "rng.hpp"

struct LsMove {
    long long delta = 0;
    int kind = 0;  // 0 none, 1 window edits, 2 two-opt*
    int nEdits = 0;
    WindowEdit e[2];
    int A = -1, ca = 0, B = -1, cb = 0;
};

struct LsParams {
    int activationMargin = 100;  // time margin around changed regions
    int posWindow = 1;           // positions q-w..q+w tried around release time
    bool verify = false;         // self-test: check every applied delta
};

class LocalSearch {
public:
    LocalSearch(const Instance& ins, Rng& rng, LsParams params);

    void attach(Solution* s, Journal* j);
    void activateAll();
    void activate(const Region& g);
    void activate(const std::vector<Region>& gs) {
        for (const Region& g : gs) activate(g);
    }
    // Runs until no dirty (flight, runway) pair is left. Returns #moves applied.
    int run();

    long long evaluations() const { return evals_; }
    Editor& editor() { return editor_; }
    const Evaluator& evaluator() const { return ev_; }

private:
    const Instance& ins_;
    Rng& rng_;
    LsParams prm_;
    Evaluator ev_;
    Editor editor_;
    Solution* sol_ = nullptr;
    std::vector<uint64_t> mask_;
    std::vector<int> queue_;
    std::vector<char> inQueue_;
    std::vector<int> sortedR_;
    std::vector<Region> regions_;
    uint64_t all_ = 0;
    int intraBit_ = 0;
    long long evals_ = 0;

    uint64_t bitOf(int k) const { return ins_.m >= 63 ? all_ : (uint64_t{1} << k); }
    bool testRunway(uint64_t mask, int k) const { return ins_.m >= 63 ? mask != 0 : ((mask >> k) & 1u); }
    bool improve(int x);
    void evalInter(int x, int A, int a, int B, long long remDelta, LsMove& best);
    void evalIntra(int x, int A, int a, LsMove& best);
    void apply(const LsMove& mv, int hint);
};
