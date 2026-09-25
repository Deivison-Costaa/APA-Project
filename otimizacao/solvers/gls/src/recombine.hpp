// Window import crossover: keep the current solution outside a release-time window and
// import the arrangement of another solution inside it.
//   W = flights with r in [T1, T2).  The current solution without W is the frame; the donor's
//   runways restricted to W give up to m chains; chains are assigned to frame runways optimally
//   (Hungarian, exact costs) and inserted at the window position; then the caller polishes
//   with local search.
#pragma once
#include <vector>

#include "state.hpp"

class WindowImport {
public:
    explicit WindowImport(State& st);
    // Applies the import (caller must use State transactions to undo). Returns false if the
    // window is empty.
    bool apply(const std::vector<std::vector<int>>& donor, int T1, int T2);

private:
    State& st_;
    int m_;
    std::vector<char> inW_;
    std::vector<std::vector<int>> chains_;
    std::vector<int> cut_, perm_;
    std::vector<ll> cost_;
    std::vector<int> buf_;
    std::vector<std::vector<int>> newSeq_;
};
