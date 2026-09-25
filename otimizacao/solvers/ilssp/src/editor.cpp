#include "editor.hpp"

#include <algorithm>
#include <utility>

void Journal::init(int m) {
    touched_.clear();
    isTouched_.assign(m, 0);
    backup_.assign(m, Runway{});
}

void Journal::touch(const Solution& s, int k) {
    if (isTouched_[k]) return;
    isTouched_[k] = 1;
    touched_.push_back(k);
    backup_[k] = s.rw[k];
}

void Journal::commit() {
    for (int k : touched_) isTouched_[k] = 0;
    touched_.clear();
}

void Journal::revert(const Instance& ins, Solution& s) {
    (void)ins;
    for (int k : touched_) {
        s.cost += backup_[k].cost() - s.rw[k].cost();
        std::swap(s.rw[k], backup_[k]);
    }
    for (int k : touched_) {
        const Runway& R = s.rw[k];
        for (int q = 0; q < R.size(); ++q) {
            const int f = R.seq[q];
            s.rwOf[f] = k;
            s.posOf[f] = q;
            s.startOf[f] = R.S[q];
        }
        isTouched_[k] = 0;
    }
    touched_.clear();
}

Region Editor::finish(int k, int lo, int structEnd, int hint) {
    const int last = sol_->recompute(ins_, k, lo, structEnd);
    const Runway& R = sol_->rw[k];
    const int len = R.size();
    if (len == 0) return Region{k, hint, hint};
    const int a = std::clamp(lo - 1, 0, len - 1);
    const int b = std::clamp(last + 1, 0, len - 1);
    return Region{k, std::min(R.S[a], hint), std::max(R.S[b], hint)};
}

void Editor::applyWindows(const WindowEdit* edits, int count, int hint, std::vector<Region>& out) {
    for (int e = 0; e < count; ++e) {
        const WindowEdit& w = edits[e];
        touch(w.k);
        std::vector<int>& seq = sol_->rw[w.k].seq;
        if (w.hi - w.lo == w.L) {
            std::copy(w.list, w.list + w.L, seq.begin() + w.lo);
        } else {
            seq.erase(seq.begin() + w.lo, seq.begin() + w.hi);
            seq.insert(seq.begin() + w.lo, w.list, w.list + w.L);
        }
    }
    for (int e = 0; e < count; ++e) {
        const WindowEdit& w = edits[e];
        out.push_back(finish(w.k, w.lo, w.lo + w.L, hint));
    }
}

void Editor::applyTwoOpt(int A, int ca, int B, int cb, int hint, std::vector<Region>& out) {
    touch(A);
    touch(B);
    std::vector<int>& sa = sol_->rw[A].seq;
    std::vector<int>& sb = sol_->rw[B].seq;
    tmpA_.assign(sa.begin() + ca, sa.end());
    tmpB_.assign(sb.begin() + cb, sb.end());
    sa.resize(ca);
    sa.insert(sa.end(), tmpB_.begin(), tmpB_.end());
    sb.resize(cb);
    sb.insert(sb.end(), tmpA_.begin(), tmpA_.end());
    out.push_back(finish(A, ca, ca + 1, hint));
    out.push_back(finish(B, cb, cb + 1, hint));
}
