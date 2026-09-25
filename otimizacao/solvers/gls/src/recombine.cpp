#include "recombine.hpp"

#include "assign.hpp"

WindowImport::WindowImport(State& st) : st_(st), m_(st.in.m) {
    inW_.assign(st.in.n, 0);
    chains_.resize(m_);
    cut_.resize(m_);
    cost_.resize(static_cast<size_t>(m_) * m_);
    newSeq_.resize(m_);
}

bool WindowImport::apply(const std::vector<std::vector<int>>& donor, int T1, int T2) {
    const auto& r = st_.in.r;
    bool any = false;
    for (int f = 0; f < st_.in.n; ++f) {
        inW_[f] = (r[f] >= T1 && r[f] < T2);
        any |= inW_[f] != 0;
    }
    if (!any) return false;
    for (int k = 0; k < m_; ++k) {
        chains_[k].clear();
        for (int f : donor[k])
            if (inW_[f]) chains_[k].push_back(f);
    }
    // frame: current solution without W
    for (int i = 0; i < m_; ++i) {
        buf_.clear();
        for (int f : st_.rw[i].seq)
            if (!inW_[f]) buf_.push_back(f);
        if (static_cast<int>(buf_.size()) != st_.rw[i].len()) st_.setRunway(i, buf_);
    }
    for (int i = 0; i < m_; ++i) {
        const auto& seq = st_.rw[i].seq;
        int c = 0;
        while (c < static_cast<int>(seq.size()) && r[seq[c]] < T1) ++c;
        cut_[i] = c;
    }
    for (int i = 0; i < m_; ++i)
        for (int j = 0; j < m_; ++j)
            cost_[static_cast<size_t>(i) * m_ + j] =
                st_.eval(i, cut_[i] - 1, chains_[j].data(), static_cast<int>(chains_[j].size()), i, cut_[i], INF_COST);
    hungarian(cost_, m_, perm_);
    for (int i = 0; i < m_; ++i) {
        const auto& seq = st_.rw[i].seq;
        const auto& ch = chains_[perm_[i]];
        newSeq_[i].assign(seq.begin(), seq.begin() + cut_[i]);
        newSeq_[i].insert(newSeq_[i].end(), ch.begin(), ch.end());
        newSeq_[i].insert(newSeq_[i].end(), seq.begin() + cut_[i], seq.end());
    }
    for (int i = 0; i < m_; ++i)
        if (!chains_[perm_[i]].empty()) st_.setRunway(i, newSeq_[i]);
    return true;
}
