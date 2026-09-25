// Granular local search driven by a queue of dirty flights (don't-look bits).
//
// For a flight f in runway A (position i) and each candidate predecessor g / successor h
// the following moves that create the arc g->f (resp. f->h) are evaluated:
//   relocate a string of 1..maxSeg flights starting (ending) at f,
//   cross-exchange of strings (1..maxCross each side),
//   2-opt* (tail exchange between runways),
//   intra-runway relocation and swap.
// Plus relocation / tail exchange to the start of other runways.
#pragma once
#include "neighbors.hpp"
#include "state.hpp"

struct LSParams {
    int maxSeg = 3;       // relocation string length
    int maxCross = 3;     // cross-exchange string length
    int kPred = 1 << 30;  // how many candidates of each list to use
    bool firstImprovement = false;
    bool prune = true;    // skip candidates whose lower bound on the move delta cannot improve
};

class LocalSearch {
public:
    LocalSearch(State& st, const Neighbors& nb, const LSParams& prm);
    // Processes the dirty queue of the state until empty. Returns number of applied moves.
    long run();
    long long evals = 0;
    long long applied = 0;
    long long flightsExamined = 0;
    long long pruned = 0;
    // statistics per move type: 0 reloc-pred 1 cross-pred 2 2opt-pred 3 reloc-succ 4 cross-succ
    // 5 2opt-succ 6 intra 7 starts
    enum { NTYPES = 8 };
    long long evalsBy[NTYPES] = {0};
    long long appliedBy[NTYPES] = {0};

private:
    bool improveFlight(int f);
    void interPred(int f, int A, int i, int g);
    void interSucc(int f, int A, int i, int h);
    void intraPred(int f, int A, int i, int q);
    void intraSucc(int f, int A, int i, int q);
    void runwayStarts(int f, int A, int i);
    // record a candidate (2 runways or 1 runway) if it beats the current best
    void record2(ll delta, int tA, int XA, int aA, const int* EA, int neA, int YA, int bA,
                 int tB, int XB, int aB, const int* EB, int neB, int YB, int bB);
    void record1(ll delta, int t, int X, int a, const int* E, int ne, int Y, int b);
    int curType_ = 0;
    int bestType_ = 0;
    bool done() const { return prm_.firstImprovement && best_.delta < 0; }

    State& st_;
    const Neighbors& nb_;
    LSParams prm_;
    Move best_;
    ll remF_[8], remB_[8];
    int tmp_[MAXE];
};
