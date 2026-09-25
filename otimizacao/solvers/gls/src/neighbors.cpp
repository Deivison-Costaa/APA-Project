#include "neighbors.hpp"

#include <algorithm>
#include <numeric>
#include <utility>

namespace {

double arcScore(const Instance& in, int i, int j, double idleWeight) {
    const int slack = in.r[j] - in.r[i] - in.gap(i, j);
    if (slack < 0) return static_cast<double>(in.p[j]) * (-slack);
    return idleWeight * slack;
}

void keepBest(std::vector<std::pair<double, int>>& cand, int K, std::vector<int>& out) {
    const int k = std::min<int>(K, static_cast<int>(cand.size()));
    std::partial_sort(cand.begin(), cand.begin() + k, cand.end());
    out.resize(k);
    for (int x = 0; x < k; ++x) out[x] = cand[x].second;
}

}  // namespace

Neighbors buildNeighbors(const Instance& in, int K, double idleWeight) {
    const int n = in.n;
    Neighbors nb;
    nb.K = K;
    nb.pred.resize(n);
    nb.succ.resize(n);
    std::vector<std::pair<double, int>> cand;
    cand.reserve(n);
    for (int f = 0; f < n; ++f) {
        cand.clear();
        for (int g = 0; g < n; ++g)
            if (g != f) cand.emplace_back(arcScore(in, g, f, idleWeight), g);
        keepBest(cand, K, nb.pred[f]);
        cand.clear();
        for (int h = 0; h < n; ++h)
            if (h != f) cand.emplace_back(arcScore(in, f, h, idleWeight), h);
        keepBest(cand, K, nb.succ[f]);
    }
    nb.byRelease.resize(n);
    std::iota(nb.byRelease.begin(), nb.byRelease.end(), 0);
    std::stable_sort(nb.byRelease.begin(), nb.byRelease.end(),
                     [&](int a, int b) { return in.r[a] < in.r[b]; });
    nb.rankOf.resize(n);
    for (int x = 0; x < n; ++x) nb.rankOf[nb.byRelease[x]] = x;
    return nb;
}
