// Granular local search: relocate / swap / 2-opt* / or-opt with neighbor lists,
// don't-look queue and sync-stop evaluation.
#pragma once
#include <vector>

#include "instance.hpp"
#include "rng.hpp"
#include "solution.hpp"

struct ConcatSpec {
    int P = -1, a = 0;  // prefix: runway P positions [0,a); P is also the runway slot rewritten
    int Q = -1, b = 0;  // suffix: runway Q positions [b, len)
    int nx = 0;
    int X[24];
};

struct Move {
    long long delta = 0;
    int type = 0;
    int nr = 0;
    ConcatSpec spec[2];
};

class LocalSearch {
public:
    LocalSearch(const Instance& inst, int K, int window);

    void push(int u);
    void pushAll(Rng& rng);
    void pushAround(const Solution& s, int k, int pos, int radius);
    void clearQueue();
    // Improve until the queue is empty. Returns total delta (<= 0).
    long long run(Solution& s, Rng& rng);
    // Apply a move (both runways built before either is replaced).
    void apply(Solution& s, const Move& mv);

    const std::vector<std::vector<int>>& neighbors() const { return near_; }
    long long evals = 0, applied = 0;
    long long byType[12] = {0};  // applied moves per type (see kMoveNames)
    int curType_ = 0;
    int sameDist = 6;
    bool orOpt = false;  // or-opt of 2-3 flights (rarely productive, ~half the evaluations)

private:
    bool improveFlight(Solution& s, int u, Move& best);
    void sameRunway(const Solution& s, int u, int A, int i, int j, Move& best);
    void consider(const Solution& s, Move& best, long long delta, int nr, const ConcatSpec& s0,
                  const ConcatSpec& s1);

    const Instance& I;
    std::vector<std::vector<int>> near_;
    std::vector<char> inQ_;
    std::vector<int> queue_;
};

std::vector<int> buildConcat(const Solution& s, const ConcatSpec& c);
