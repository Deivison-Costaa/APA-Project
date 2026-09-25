#include "assign.hpp"

#include <algorithm>

AssignNeighborhood::AssignNeighborhood(State& st) : st_(st), m_(st.in.m) {
    cost_.resize(static_cast<size_t>(m_) * m_);
    cutA_.resize(m_);
    cutB_.resize(m_);
    perm_.resize(m_);
    newSeq_.resize(m_);
    for (auto& s : newSeq_) s.reserve(st.in.n);
}

void AssignNeighborhood::cutsAt(int T, std::vector<int>& idx) const {
    for (int k = 0; k < m_; ++k) {
        const auto& S = st_.rw[k].S;
        idx[k] = static_cast<int>(std::lower_bound(S.begin(), S.end(), T) - S.begin());
    }
}

void AssignNeighborhood::jitter(std::vector<int>& idx) {
    if (!rng_) return;
    for (int k = 0; k < m_; ++k) {
        const int len = st_.rw[k].len();
        idx[k] = std::clamp(idx[k] + rng_->range(-1, 1), 0, len);
    }
}

ll AssignNeighborhood::solve(std::vector<int>& perm) { return hungarian(cost_, m_, perm); }

// Hungarian algorithm (shortest augmenting paths with potentials), minimization. O(m^3).
ll hungarian(const std::vector<ll>& cost, int n, std::vector<int>& perm) {
    std::vector<ll> u(n + 1, 0), v(n + 1, 0), minv(n + 1);
    std::vector<int> p(n + 1, 0), way(n + 1, 0);
    std::vector<char> used(n + 1);
    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        std::fill(minv.begin(), minv.end(), INF_COST);
        std::fill(used.begin(), used.end(), 0);
        do {
            used[j0] = 1;
            const int i0 = p[j0];
            ll delta = INF_COST;
            int j1 = 0;
            for (int j = 1; j <= n; ++j) {
                if (used[j]) continue;
                const ll cur = cost[static_cast<size_t>(i0 - 1) * n + (j - 1)] - u[i0] - v[j];
                if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                if (minv[j] < delta) { delta = minv[j]; j1 = j; }
            }
            for (int j = 0; j <= n; ++j) {
                if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                else minv[j] -= delta;
            }
            j0 = j1;
        } while (p[j0] != 0);
        do {
            const int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }
    perm.resize(n);
    ll total = 0;
    for (int j = 1; j <= n; ++j) {
        perm[p[j] - 1] = j - 1;
        total += cost[static_cast<size_t>(p[j] - 1) * n + (j - 1)];
    }
    return total;
}

ll AssignNeighborhood::tailAt(int T) {
    cutsAt(T, cutB_);
    jitter(cutB_);
    for (int k = 0; k < m_; ++k) cutA_[k] = cutB_[k] - 1;
    ll cur = 0;
    for (int i = 0; i < m_; ++i) {
        for (int j = 0; j < m_; ++j)
            cost_[static_cast<size_t>(i) * m_ + j] = st_.eval(i, cutA_[i], nullptr, 0, j, cutB_[j], INF_COST);
        cur += cost_[static_cast<size_t>(i) * m_ + i];
    }
    evals += static_cast<long long>(m_) * m_;
    const ll best = solve(perm_);
    if (best >= cur) return 0;
    for (int i = 0; i < m_; ++i) {
        const auto& P = st_.rw[i].seq;
        const auto& Q = st_.rw[perm_[i]].seq;
        newSeq_[i].assign(P.begin(), P.begin() + cutA_[i] + 1);
        newSeq_[i].insert(newSeq_[i].end(), Q.begin() + cutB_[perm_[i]], Q.end());
    }
    for (int i = 0; i < m_; ++i)
        if (perm_[i] != i) st_.setRunway(i, newSeq_[i]);
    return best - cur;
}

ll AssignNeighborhood::segmentAt(int T1, int T2) {
    cutsAt(T1, cutA_);
    cutsAt(T2, cutB_);
    jitter(cutA_);
    jitter(cutB_);
    for (int k = 0; k < m_; ++k) {
        cutB_[k] = std::max(cutB_[k], cutA_[k]);
        cutA_[k] -= 1;  // last kept prefix position
    }
    ll cur = 0;
    for (int i = 0; i < m_; ++i) {
        for (int j = 0; j < m_; ++j) {
            const int* seg = st_.rw[j].seq.data() + cutA_[j] + 1;
            const int len = cutB_[j] - cutA_[j] - 1;
            cost_[static_cast<size_t>(i) * m_ + j] = st_.eval(i, cutA_[i], seg, len, i, cutB_[i], INF_COST);
        }
        cur += cost_[static_cast<size_t>(i) * m_ + i];
    }
    evals += static_cast<long long>(m_) * m_;
    const ll best = solve(perm_);
    if (best >= cur) return 0;
    for (int i = 0; i < m_; ++i) {
        const auto& P = st_.rw[i].seq;
        const auto& Q = st_.rw[perm_[i]].seq;
        newSeq_[i].assign(P.begin(), P.begin() + cutA_[i] + 1);
        newSeq_[i].insert(newSeq_[i].end(), Q.begin() + cutA_[perm_[i]] + 1, Q.begin() + cutB_[perm_[i]]);
        newSeq_[i].insert(newSeq_[i].end(), P.begin() + cutB_[i], P.end());
    }
    for (int i = 0; i < m_; ++i)
        if (perm_[i] != i) st_.setRunway(i, newSeq_[i]);
    return best - cur;
}
