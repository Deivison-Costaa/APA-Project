// Granular local search with incremental (sync-stop) runway evaluation.
//
// Every move is expressed as at most two "edits". An edit rewrites one runway as
//     runway[0..a] + L[0..nL) + tail(route tr, from position tp to its end)
// Its cost delta is sum p_x (S'_x - S_x) over the flights of L and of the tail, and
// the tail scan stops as soon as a flight keeps its old start time (the remainder of
// that old sequence is then unchanged). Deltas of the two edits of a move add up.
#pragma once
#include <vector>

#include "instance.hpp"
#include "params.hpp"

class LocalSearch {
public:
    static constexpr int MAXL = 24;
    struct Edit {
        int route, a;  // keep route[0..a] (a = -1: nothing)
        int L[MAXL];
        int nL;
        int tr, tp;  // then route tr from position tp to the end
    };

    LocalSearch(const Instance& ins, const Params& par);

    // Loads a (possibly partial) solution; flights absent from `routes` are unassigned.
    void load(const Routes& routes);
    // Inserts each unassigned flight at its cheapest position (in the given order).
    void insertMissing(const std::vector<int>& flights);
    // Runs the descent until no improving move exists. Returns the final cost.
    // With `dirty`, only pairs near flagged flights are examined at first (the rest of the
    // solution is assumed to be locally optimal already).
    long long run(Rng& rng, const std::vector<char>* dirty = nullptr);

    long long cost() const { return total_; }
    const Routes& routes() const { return R_; }
    const std::vector<int>& starts() const { return S_; }
    const std::vector<int>& preds() const { return pred_; }
    const std::vector<int>& succs() const { return succ_; }
    long long movesApplied() const { return nbMoves_; }
    long long evaluations() const { return nbEvals_; }
    // Multi-runway tail reassignment: at every cut time, the optimal matching of runway
    // heads to runway tails (Hungarian). Returns true if it improved the solution.
    bool cutPass();
    // Debug: recompute everything and compare with incremental data.
    bool consistent() const;

private:
    const Instance& I;
    const Params& P;
    int n, m;
    const int *r, *c, *p;
    const SetupT* t;
    // cached removal deltas of the segments starting at the flight being examined
    int remFor_ = -1;
    long long remStamp_ = -1;
    long long version_ = 0;  // bumped on every change of the solution (never reset)
    long long rem_[4];

    Routes R_;
    std::vector<int> rt_, pos_, S_, pred_, succ_;
    long long total_ = 0;

    std::vector<std::vector<int>> neigh_;
    std::vector<long long> lastTested_, nodeMod_;
    long long stamp_ = 1;
    std::vector<int> order_;
    std::vector<std::vector<int>> nvBuf_;
    long long nbMoves_ = 0, nbEvals_ = 0;

    void buildNeighbours();
    void refreshRoute(int k, int from, bool mark);
    long long windowMod(int x) const;
    long long evalEdit(const Edit& e);
    void applyEdits(const Edit* e, int ne);
    bool tryPair(int u, int v);
    bool tryInter(int u, int v);
    bool tryIntra(int u, int v);
    bool tryEjection(int u, int v);
    bool tryEmpty(int u);
    bool commitIfBetter(Edit* e, int ne);
    bool commitWithKnown(long long d0, Edit* e, int ne);  // e[0] already evaluated as d0
    long long removalDelta(int u, int len);                // remove len flights starting at u
};
