// Assignment-based large neighborhoods (exact over all runway permutations):
//  * tail assignment: cut every runway at time T and re-match the m prefixes with the
//    m tails optimally (a cyclic generalization of 2-opt*),
//  * segment assignment: cut every runway at T1 and T2 and re-assign the m middle
//    segments to the m (prefix, suffix) frames optimally (cyclic cross-exchange).
// Both solve an m x m assignment problem (Hungarian, O(m^3)) whose costs are evaluated
// exactly with the sync-stop evaluator.
#pragma once
#include <vector>

#include "common.hpp"
#include "state.hpp"

class AssignNeighborhood {
public:
    explicit AssignNeighborhood(State& st);
    // returns the improvement applied (<= 0 means nothing applied)
    ll tailAt(int T);
    ll segmentAt(int T1, int T2);
    // cut indices may be jittered by +-1 position per runway
    void setJitter(Rng* rng) { rng_ = rng; }
    long long evals = 0;

private:
    void cutsAt(int T, std::vector<int>& idx) const;  // first position with S >= T per runway
    ll solve(std::vector<int>& perm);                 // Hungarian on cost_ (m x m)
    void jitter(std::vector<int>& idx);

    State& st_;
    int m_;
    std::vector<ll> cost_;
    std::vector<int> cutA_, cutB_, perm_;
    std::vector<std::vector<int>> newSeq_;
    Rng* rng_ = nullptr;
};

// Hungarian algorithm on an m x m cost matrix (row-major). perm[row] = assigned column.
ll hungarian(const std::vector<ll>& cost, int m, std::vector<int>& perm);
