// Solution state with O(1)-amortized move evaluation.
//
// A candidate runway is always described as   X[0..a] + E[0..ne) + Y[b..]
// (prefix of runway X, explicit flights, suffix of runway Y). Its cost is
//   cum_X[a+1] + cost(E) + recomputed suffix,
// and the suffix recomputation stops as soon as a flight gets the same start time
// it currently has in Y ("sync"): from there on the schedule is unchanged, so the
// remaining cost is read from Y's prefix-cost array.
#pragma once
#include <algorithm>
#include <vector>

#include "common.hpp"
#include "instance.hpp"

constexpr int MAXE = 40;  // max explicit flights in a piece

struct Runway {
    std::vector<int> seq;  // flights in order
    std::vector<int> S;    // start time per position
    std::vector<ll> cum;   // cum[k] = cost of the first k flights (size len+1)
    int len() const { return static_cast<int>(seq.size()); }
    ll cost() const { return cum[seq.size()]; }
};

// One new runway content: target <- X[0..a] + E + Y[b..]   (X<0 or a<0: no prefix; Y<0: no suffix)
struct Piece {
    int target = -1, X = -1, a = -1, Y = -1, b = 0, ne = 0;
    int E[MAXE];
};

struct Move {
    int np = 0;
    ll delta = 0;
    Piece part[2];
};

class State {
public:
    explicit State(const Instance& in);

    void load(const std::vector<std::vector<int>>& seqs);
    std::vector<std::vector<int>> sequences() const;

    // cost of X[0..a] + E + Y[b..]; returns INF_COST as soon as the partial cost exceeds limit
    inline ll eval(int X, int a, const int* E, int ne, int Y, int b, ll limit) const;
    void build(const Piece& pc, std::vector<int>& out) const;

    void apply(const Move& mv);
    void setRunway(int k, std::vector<int>& newSeq);  // swaps newSeq in; newSeq gets old content

    // transactions (undo of all runway changes since begin())
    void begin();
    void rollback();

    // dirty queue: flights whose predecessor/successor/start changed
    bool trackDirty = true;
    std::vector<int> dirty;
    std::vector<char> isDirty;
    void markDirty(int f) {
        if (!isDirty[f]) { isDirty[f] = 1; dirty.push_back(f); }
    }
    void clearDirty() {
        for (int f : dirty) isDirty[f] = 0;
        dirty.clear();
    }

    const Instance& in;
    std::vector<Runway> rw;
    std::vector<int> where, pos, SF, prevF, nextF;
    ll total = 0;

private:
    void rebuild(int k);
    void touch(int k);
    std::vector<std::vector<int>> backup_;
    std::vector<int> savedStamp_;
    std::vector<int> touched_;
    int stamp_ = 1;
    bool inTxn_ = false;
    std::vector<int> buf0_, buf1_;
};

inline ll State::eval(int X, int a, const int* E, int ne, int Y, int b, ll limit) const {
    const int* r = in.r.data();
    const int* p = in.p.data();
    ll cost = 0;
    int last = -1, st = 0;
    if (a >= 0) {
        const Runway& R = rw[X];
        cost = R.cum[a + 1];
        last = R.seq[a];
        st = R.S[a];
    }
    for (int e = 0; e < ne; ++e) {
        const int f = E[e];
        const int s = last < 0 ? r[f] : std::max(r[f], st + in.gap(last, f));
        cost += static_cast<ll>(p[f]) * (s - r[f]);
        last = f;
        st = s;
    }
    if (cost > limit) return INF_COST;
    if (Y >= 0) {
        const Runway& R = rw[Y];
        const int len = R.len();
        const int* seq = R.seq.data();
        const int* S = R.S.data();
        for (int k = b; k < len; ++k) {
            const int f = seq[k];
            const int s = last < 0 ? r[f] : std::max(r[f], st + in.gap(last, f));
            if (s == S[k]) {
                cost += R.cum[len] - R.cum[k];
                return cost > limit ? INF_COST : cost;
            }
            cost += static_cast<ll>(p[f]) * (s - r[f]);
            if (cost > limit) return INF_COST;
            last = f;
            st = s;
        }
    }
    return cost;
}
