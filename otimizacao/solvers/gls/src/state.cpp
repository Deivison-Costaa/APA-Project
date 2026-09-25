#include "state.hpp"

#include <stdexcept>

State::State(const Instance& inst) : in(inst) {
    const int n = in.n;
    rw.resize(in.m);
    for (auto& R : rw) {
        R.seq.reserve(n);
        R.S.reserve(n);
        R.cum.reserve(n + 1);
        R.cum.assign(1, 0);
    }
    where.assign(n, -1);
    pos.assign(n, -1);
    SF.assign(n, -1);
    prevF.assign(n, -2);
    nextF.assign(n, -2);
    isDirty.assign(n, 0);
    backup_.resize(in.m);
    for (auto& b : backup_) b.reserve(n);
    savedStamp_.assign(in.m, 0);
    buf0_.reserve(n);
    buf1_.reserve(n);
}

void State::load(const std::vector<std::vector<int>>& seqs) {
    if (static_cast<int>(seqs.size()) != in.m) throw std::runtime_error("load: wrong number of runways");
    total = 0;
    for (int k = 0; k < in.m; ++k) {
        rw[k].seq = seqs[k];
        rw[k].cum.assign(1, 0);
        rw[k].S.clear();
    }
    for (int k = 0; k < in.m; ++k) rebuild(k);
    for (int f = 0; f < in.n; ++f)
        if (where[f] < 0) throw std::runtime_error("load: missing flight");
}

std::vector<std::vector<int>> State::sequences() const {
    std::vector<std::vector<int>> out(in.m);
    for (int k = 0; k < in.m; ++k) out[k] = rw[k].seq;
    return out;
}

void State::rebuild(int k) {
    Runway& R = rw[k];
    const ll oldCost = R.cum.empty() ? 0 : R.cum.back();
    const int len = R.len();
    R.S.resize(len);
    R.cum.resize(len + 1);
    R.cum[0] = 0;
    int last = -1, st = 0;
    ll cost = 0;
    for (int j = 0; j < len; ++j) {
        const int f = R.seq[j];
        const int s = last < 0 ? in.r[f] : std::max(in.r[f], st + in.gap(last, f));
        cost += static_cast<ll>(in.p[f]) * (s - in.r[f]);
        R.S[j] = s;
        R.cum[j + 1] = cost;
        where[f] = k;
        pos[f] = j;
        const int nx = j + 1 < len ? R.seq[j + 1] : -1;
        if (SF[f] != s || prevF[f] != last || nextF[f] != nx) {
            SF[f] = s;
            prevF[f] = last;
            nextF[f] = nx;
            if (trackDirty) markDirty(f);
        }
        last = f;
        st = s;
    }
    total += cost - oldCost;
}

void State::touch(int k) {
    if (inTxn_ && savedStamp_[k] != stamp_) {
        savedStamp_[k] = stamp_;
        backup_[k] = rw[k].seq;
        touched_.push_back(k);
    }
}

void State::setRunway(int k, std::vector<int>& newSeq) {
    touch(k);
    rw[k].seq.swap(newSeq);
    rebuild(k);
}

void State::begin() {
    ++stamp_;
    touched_.clear();
    inTxn_ = true;
}

void State::rollback() {
    const bool td = trackDirty;
    trackDirty = false;
    for (int k : touched_) {
        rw[k].seq.swap(backup_[k]);
        rebuild(k);
    }
    trackDirty = td;
    touched_.clear();
    ++stamp_;
}

void State::build(const Piece& pc, std::vector<int>& out) const {
    out.clear();
    if (pc.a >= 0) out.insert(out.end(), rw[pc.X].seq.begin(), rw[pc.X].seq.begin() + pc.a + 1);
    out.insert(out.end(), pc.E, pc.E + pc.ne);
    if (pc.Y >= 0) out.insert(out.end(), rw[pc.Y].seq.begin() + pc.b, rw[pc.Y].seq.end());
}

void State::apply(const Move& mv) {
    build(mv.part[0], buf0_);
    if (mv.np > 1) build(mv.part[1], buf1_);
    setRunway(mv.part[0].target, buf0_);
    if (mv.np > 1) setRunway(mv.part[1].target, buf1_);
}
