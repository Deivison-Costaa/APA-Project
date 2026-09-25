#include "perturb.hpp"

#include <algorithm>
#include <numeric>

Perturber::Perturber(State& st, const Neighbors& nb, const PerturbParams& prm)
    : st_(st), nb_(nb), prm_(prm) {
    mark_.assign(st.in.n, 0);
    runwayMark_.assign(st.in.m, 0);
    buf_.reserve(st.in.n);
    removed_.reserve(st.in.n);
    key_.resize(st.in.n);
}

int Perturber::pickSeed(Rng& rng) {
    const int n = st_.in.n;
    if (rng.uniform() < prm_.pDelayedSeed) {
        for (int tries = 0; tries < 32; ++tries) {
            const int f = rng.below(n);
            if (st_.SF[f] > st_.in.r[f]) return f;
        }
    }
    return rng.below(n);
}

void Perturber::ruinWindow(int f0, int k, Rng& rng) {
    const int n = st_.in.n;
    const int rk = nb_.rankOf[f0];
    int lo = rk - rng.below(k);
    lo = std::clamp(lo, 0, std::max(0, n - k));
    const int hi = std::min(n - 1, lo + k - 1);
    for (int x = lo; x <= hi; ++x) removed_.push_back(nb_.byRelease[x]);
}

void Perturber::ruinStrings(int f0, int k, Rng& rng) {
    const int m = st_.in.m;
    const int T = st_.SF[f0];
    const int nr = std::min(m, rng.range(2, std::max(2, prm_.maxRunways)));
    // runways: that of f0 plus runways of random candidate neighbors, then random ones
    std::vector<int> rws;
    rws.push_back(st_.where[f0]);
    runwayMark_[rws[0]] = 1;
    const auto& P = nb_.pred[f0];
    const auto& S = nb_.succ[f0];
    for (int tries = 0; tries < 40 && static_cast<int>(rws.size()) < nr; ++tries) {
        const auto& L = (tries & 1) ? S : P;
        if (L.empty()) break;
        const int g = L[rng.below(std::min<int>(10, static_cast<int>(L.size())))];
        const int B = st_.where[g];
        if (!runwayMark_[B]) { runwayMark_[B] = 1; rws.push_back(B); }
    }
    for (int tries = 0; tries < 40 && static_cast<int>(rws.size()) < nr; ++tries) {
        const int B = rng.below(m);
        if (!runwayMark_[B]) { runwayMark_[B] = 1; rws.push_back(B); }
    }
    for (int B : rws) runwayMark_[B] = 0;
    const int per = std::max(1, k / static_cast<int>(rws.size()));
    for (int B : rws) {
        const Runway& R = st_.rw[B];
        const int len = R.len();
        if (len == 0) continue;
        const int l = std::min(len, std::max(1, per + rng.range(-1, 1)));
        const int idx = static_cast<int>(std::lower_bound(R.S.begin(), R.S.end(), T) - R.S.begin());
        int start = idx - rng.below(l);
        start = std::clamp(start, 0, len - l);
        for (int x = start; x < start + l; ++x) removed_.push_back(R.seq[x]);
    }
}

void Perturber::removeMarked() {
    std::vector<int> rws;
    for (int f : removed_) {
        mark_[f] = 1;
        const int B = st_.where[f];
        if (!runwayMark_[B]) { runwayMark_[B] = 1; rws.push_back(B); }
    }
    for (int B : rws) {
        runwayMark_[B] = 0;
        buf_.clear();
        for (int f : st_.rw[B].seq)
            if (!mark_[f]) buf_.push_back(f);
        st_.setRunway(B, buf_);
    }
    for (int f : removed_) mark_[f] = 0;
}

void Perturber::orderRemoved(Rng& rng) {
    const int mode = rng.below(4);
    if (mode == 0) {  // random
        for (int x = static_cast<int>(removed_.size()) - 1; x > 0; --x) std::swap(removed_[x], removed_[rng.below(x + 1)]);
        return;
    }
    for (int f : removed_) {
        const double noise = rng.uniform();
        if (mode == 1) key_[f] = st_.in.r[f] + 20.0 * noise;          // by release (noisy)
        else if (mode == 2) key_[f] = -st_.in.p[f] - 10.0 * noise;    // by penalty desc
        else key_[f] = -(st_.in.r[f] + 20.0 * noise);                 // reverse release
    }
    std::sort(removed_.begin(), removed_.end(), [&](int a, int b) { return key_[a] < key_[b]; });
}

void Perturber::insertBest(int f, Rng& rng, double blink) {
    const int m = st_.in.m;
    const int rf = st_.in.r[f];
    ll bestInc = INF_COST;
    int bestB = -1, bestQ = -1;
    for (int pass = 0; pass < 2 && bestB < 0; ++pass) {
        for (int B = 0; B < m; ++B) {
            const Runway& R = st_.rw[B];
            const int len = R.len();
            const ll costB = R.cost();
            const int idx = static_cast<int>(std::lower_bound(R.S.begin(), R.S.end(), rf) - R.S.begin());
            const int lo = std::max(0, idx - prm_.insWin), hi = std::min(len, idx + prm_.insWin);
            for (int q = lo; q <= hi; ++q) {
                if (pass == 0 && blink > 0 && rng.uniform() < blink) continue;
                const ll lim = bestInc >= INF_COST ? INF_COST : costB + bestInc - 1;
                const ll nw = st_.eval(B, q - 1, &f, 1, B, q, lim);
                if (nw >= INF_COST) continue;
                if (nw - costB < bestInc) {
                    bestInc = nw - costB;
                    bestB = B;
                    bestQ = q;
                }
            }
        }
    }
    const Runway& R = st_.rw[bestB];
    buf_.assign(R.seq.begin(), R.seq.begin() + bestQ);
    buf_.push_back(f);
    buf_.insert(buf_.end(), R.seq.begin() + bestQ, R.seq.end());
    st_.setRunway(bestB, buf_);
}

void Perturber::ruinRecreate(Rng& rng) {
    removed_.clear();
    const int f0 = pickSeed(rng);
    center_ = st_.SF[f0];
    const int k = rng.range(prm_.kmin, prm_.kmax);
    if (rng.uniform() < prm_.pWindow) ruinWindow(f0, k, rng);
    else ruinStrings(f0, k, rng);
    removeMarked();
    orderRemoved(rng);
    for (int f : removed_) insertBest(f, rng, prm_.blink);
}
