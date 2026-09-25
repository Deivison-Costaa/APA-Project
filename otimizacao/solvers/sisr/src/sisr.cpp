#include "sisr.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>

double nowSeconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

SisrSolver::SisrSolver(const Instance& I, const Params& P, int threadId)
    : I_(I), P_(P), tid_(threadId), rng_(P.seed * 1000003ULL + threadId * 7919ULL + 17) {
    touched_.assign(I.m, 0);
    saved_.assign(I.m, Runway{});
    blinkThresh_ = static_cast<uint32_t>(std::min(1.0, std::max(0.0, P.blink)) * 4294967295.0);
    buildAdjacency();
    byR_.resize(I.n);
    std::iota(byR_.begin(), byR_.end(), 0);
    std::stable_sort(byR_.begin(), byR_.end(), [&](int a, int b) { return I.r[a] < I.r[b]; });
    seedLo_ = 0;
    seedHi_ = I.n;
    cur_.init(I, Seqs(I.m));
}

void SisrSolver::buildAdjacency() {
    const int n = I_.n;
    const int K = std::min(P_.adjK, n - 1);
    adj_.assign(n, {});
    std::vector<int> idx(n);
    for (int s = 0; s < n; ++s) {
        std::iota(idx.begin(), idx.end(), 0);
        auto key = [&](int i) { return std::abs(I_.r[i] - I_.r[s]); };
        std::partial_sort(idx.begin(), idx.begin() + K + 1, idx.end(),
                          [&](int a, int b) { return key(a) < key(b) || (key(a) == key(b) && a < b); });
        for (int x = 0; x <= K && x < n; ++x)
            if (idx[x] != s && static_cast<int>(adj_[s].size()) < K) adj_[s].push_back(idx[x]);
    }
}

void SisrSolver::setInitial(const Seqs& seqs) {
    cur_.init(I_, seqs);
    best_ = cur_.seqs();
    bestCost_ = cur_.cost;
}

void SisrSolver::construct() {
    cur_.init(I_, Seqs(I_.m));
    removed_.resize(I_.n);
    std::iota(removed_.begin(), removed_.end(), 0);
    std::stable_sort(removed_.begin(), removed_.end(), [&](int a, int b) { return I_.r[a] < I_.r[b]; });
    recreate(false);
    touchedList_.clear();
    std::fill(touched_.begin(), touched_.end(), 0);
    best_ = cur_.seqs();
    bestCost_ = cur_.cost;
}

void SisrSolver::touch(int k) {
    if (touched_[k]) return;
    touched_[k] = 1;
    touchedList_.push_back(k);
    saved_[k] = cur_.rw[k];
}

void SisrSolver::restore() {
    for (int k : touchedList_) {
        std::swap(cur_.rw[k], saved_[k]);
        for (int f : cur_.rw[k].seq) cur_.rwOf[f] = k;
        touched_[k] = 0;
    }
    touchedList_.clear();
}

void SisrSolver::commit() {
    for (int k : touchedList_) touched_[k] = 0;
    touchedList_.clear();
}

void SisrSolver::removeRange(int k, int a, int b) {
    Runway& R = cur_.rw[k];
    for (int y = a; y < b; ++y) {
        removed_.push_back(R.seq[y]);
        cur_.rwOf[R.seq[y]] = -1;
    }
    cur_.cost += R.eraseRange(I_, a, b);
}

int SisrSolver::pickSeed() {
    const int span = seedHi_ - seedLo_;
    int s = byR_[seedLo_ + rng_.below(span)];
    if (rng_.u01() >= P_.seedBias) return s;
    // Rejection sampling towards delayed flights (or flights right before a delayed one).
    for (int tries = 0; tries < 8; ++tries) {
        const Runway& R = cur_.rw[cur_.rwOf[s]];
        const int x = R.find(s);
        if (R.S[x] > I_.r[s] || (x + 1 < R.size() && R.S[x + 1] > I_.r[R.seq[x + 1]])) return s;
        s = byR_[seedLo_ + rng_.below(span)];
    }
    return s;
}

void SisrSolver::ruinStrings(int seed) {
    int nonEmpty = 0;
    for (const auto& R : cur_.rw) nonEmpty += R.size() > 0;
    const double avgLen = static_cast<double>(I_.n) / std::max(1, nonEmpty);
    const double lsMax = std::min(static_cast<double>(P_.lmax), avgLen);
    const double ksMax = 4.0 * P_.cbar / (1.0 + lsMax) - 1.0;
    const int ks = static_cast<int>(std::floor(rng_.u01() * ksMax)) + 1;

    const std::vector<int>& nb = adj_[seed];
    int done = 0;
    for (int idx = -1; idx < static_cast<int>(nb.size()) && done < ks; ++idx) {
        const int f = idx < 0 ? seed : nb[idx];
        const int k = cur_.rwOf[f];
        if (k < 0 || touched_[k]) continue;
        const Runway& R = cur_.rw[k];
        const int L = R.size();
        const int x = R.find(f);
        const int lMaxR = std::max(1, std::min(L, static_cast<int>(lsMax)));
        const int l = rng_.below(lMaxR) + 1;
        touch(k);
        ++done;
        if (rng_.u01() < P_.splitProb && L > l + 1) {
            int keep = 1;
            while (l + keep < L && rng_.u01() < P_.keepProb) ++keep;
            const int w = l + keep;
            const int lo = std::max(0, x - w + 1), hi = std::min(x, L - w);
            const int st = lo + rng_.below(hi - lo + 1);
            const int off = rng_.below(l + 1);  // removed before the preserved block
            removeRange(k, st + off + keep, st + w);  // higher range first
            removeRange(k, st, st + off);
            cuts_.push_back({k, st});
        } else {
            const int lo = std::max(0, x - l + 1), hi = std::min(x, L - l);
            const int st = lo + rng_.below(hi - lo + 1);
            removeRange(k, st, st + l);
            cuts_.push_back({k, st});
        }
    }
}

// Remove every flight starting in [tau - w, tau + w] on a random subset of runways.
void SisrSolver::ruinSlice(int seed) {
    const int m = I_.m;
    const int tau = I_.r[seed];
    const int w = 1 + rng_.below(std::max(1, P_.sliceW));
    const int kr = 2 + rng_.below(std::max(1, m - 1));  // 2..m runways
    const int home = cur_.rwOf[seed];
    perm_.resize(m);
    std::iota(perm_.begin(), perm_.end(), 0);
    std::swap(perm_[0], perm_[home]);
    for (int i = m - 1; i > 1; --i) std::swap(perm_[i], perm_[1 + rng_.below(i)]);
    for (int i = 0; i < kr && i < m; ++i) {
        const int k = perm_[i];
        const Runway& R = cur_.rw[k];
        const int a = R.lowerBoundStart(tau - w);
        const int b = R.lowerBoundStart(tau + w + 1);
        touch(k);
        if (a < b) removeRange(k, a, b);
        cuts_.push_back({k, a});
    }
}

void SisrSolver::ruin() {
    removed_.clear();
    const int seed = pickSeed();
    if (rng_.u01() < P_.sliceProb) ruinSlice(seed);
    else ruinStrings(seed);
    const int nc = static_cast<int>(cuts_.size());
    if (nc >= 2 && rng_.u01() < P_.tailProb) {
        const int a = rng_.below(nc);
        int b = rng_.below(nc - 1);
        if (b >= a) ++b;
        swapTails(cuts_[a].first, cuts_[a].second, cuts_[b].first, cuts_[b].second);
    }
    cuts_.clear();
}

// A = A[0..ca) + B[cb..), B = B[0..cb) + A[ca..). Both runways must be touched.
void SisrSolver::swapTails(int ka, int ca, int kb, int cb) {
    Runway& A = cur_.rw[ka];
    Runway& B = cur_.rw[kb];
    auto swapVec = [&](std::vector<int>& va, std::vector<int>& vb) {
        tmp_.assign(va.begin() + ca, va.end());
        va.resize(ca);
        va.insert(va.end(), vb.begin() + cb, vb.end());
        vb.resize(cb);
        vb.insert(vb.end(), tmp_.begin(), tmp_.end());
    };
    swapVec(A.seq, B.seq);
    swapVec(A.S, B.S);
    swapVec(A.F, B.F);
    for (int x = ca; x < A.size(); ++x) cur_.rwOf[A.seq[x]] = ka;
    for (int x = cb; x < B.size(); ++x) cur_.rwOf[B.seq[x]] = kb;
    const long long before = A.cost + B.cost;
    const long long after = A.rebuild(I_) + B.rebuild(I_);
    cur_.cost += after - before;
}

void SisrSolver::orderRemoved() {
    const int total = P_.wRandom + P_.wRAsc + P_.wPDesc + P_.wRDesc;
    int pick = rng_.below(std::max(1, total));
    auto& v = removed_;
    if ((pick -= P_.wRandom) < 0) {
        for (int i = static_cast<int>(v.size()) - 1; i > 0; --i) std::swap(v[i], v[rng_.below(i + 1)]);
    } else if ((pick -= P_.wRAsc) < 0) {
        std::sort(v.begin(), v.end(), [&](int a, int b) { return I_.r[a] < I_.r[b]; });
    } else if ((pick -= P_.wPDesc) < 0) {
        std::sort(v.begin(), v.end(), [&](int a, int b) { return I_.p[a] > I_.p[b]; });
    } else {
        std::sort(v.begin(), v.end(), [&](int a, int b) { return I_.r[a] > I_.r[b]; });
    }
}

void SisrSolver::recreate(bool allowBlink) {
    const int m = I_.m;
    for (int j : removed_) {
        const int rj = I_.r[j];
        const long long pj = I_.p[j];
        const int tmin = I_.tminIn[j];
        long long bestD = kInf;
        int bk = -1, bp = -1;
        for (int pass = 0; pass < 2 && bk < 0; ++pass) {
            const bool blink = allowBlink && pass == 0;
            const int k0 = rng_.below(m);
            for (int kk = 0; kk < m; ++kk) {
                const int k = (k0 + kk) % m;
                const Runway& R = cur_.rw[k];
                const int L = R.size();
                const int lo = R.lowerBoundStart(rj - P_.window);
                for (int pos = lo; pos <= L; ++pos) {
                    if (pos > lo && pos > 0) {
                        const long long lbj = pj * std::max(0, R.F[pos - 1] + tmin - rj);
                        if (lbj >= bestD) break;
                    }
                    if (blink && rng_.next32() < blinkThresh_) continue;
                    const long long d = R.evalInsert(I_, j, pos, bestD);
                    if (d < bestD) {
                        bestD = d;
                        bk = k;
                        bp = pos;
                    }
                }
            }
        }
        touch(bk);
        cur_.cost += cur_.rw[bk].insert(I_, j, bp);
        cur_.rwOf[j] = bk;
    }
    removed_.clear();
}

bool SisrSolver::verify() const {
    long long total = 0;
    for (int k = 0; k < I_.m; ++k) {
        Runway copy = cur_.rw[k];
        const long long c = copy.rebuild(I_);
        if (c != cur_.rw[k].cost || copy.S != cur_.rw[k].S || copy.F != cur_.rw[k].F) return false;
        for (int f : cur_.rw[k].seq)
            if (cur_.rwOf[f] != k) return false;
        total += c;
    }
    return total == cur_.cost && total == evaluate(I_, cur_.seqs());
}

// Best 2-opt* (tail exchange) around a seed flight, accepted with the SA rule.
void SisrSolver::stepTail(double T, RunStats& st) {
    const int s = pickSeed();
    const int ka = cur_.rwOf[s];
    const Runway& A = cur_.rw[ka];
    const int ca = A.find(s) + static_cast<int>(rng_.next32() & 1u);
    const int tau = ca > 0 ? A.F[ca - 1] : I_.r[s];
    const double thr = -T * std::log(rng_.u01());
    const long long limit = static_cast<long long>(std::floor(thr)) + 1;  // accept iff delta < thr
    long long bestD = kInf;
    int bk = -1, bc = -1;
    for (int kb = 0; kb < I_.m; ++kb) {
        if (kb == ka) continue;
        const Runway& B = cur_.rw[kb];
        const int c0 = B.lowerBoundStart(tau);
        for (int cb = std::max(0, c0 - 1); cb <= std::min(B.size(), c0 + 1); ++cb) {
            if (rng_.next32() < blinkThresh_) continue;
            const long long d = evalTailSwap(I_, A, ca, B, cb, std::min(bestD, limit));
            if (d < bestD) {
                bestD = d;
                bk = kb;
                bc = cb;
            }
        }
    }
    ++st.iters;
    ++st.tailMoves;
    if (bk < 0 || static_cast<double>(bestD) >= thr) return;
    touch(ka);
    touch(bk);
    const long long before = cur_.cost;
    swapTails(ka, ca, bk, bc);
    commit();
    ++st.accepted;
    if (P_.selfCheck && (cur_.cost != before + bestD || !verify())) {
        std::fprintf(stderr, "SELF-CHECK FAILED (tail) at iter %lld\n", st.iters);
        std::abort();
    }
    if (cur_.cost < bestCost_) {
        bestCost_ = cur_.cost;
        best_ = cur_.seqs();
        ++st.improvements;
        shared_->offer(bestCost_, best_);
    }
}

void SisrSolver::step(double T, RunStats& st) {
    if (P_.optProb > 0 && rng_.u01() < P_.optProb) {
        stepTail(T, st);
        return;
    }
    ++st.iters;
    const long long before = cur_.cost;
    ruin();
    orderRemoved();
    recreate(true);
    if (P_.selfCheck && !verify()) {
        std::fprintf(stderr, "SELF-CHECK FAILED at iter %lld\n", st.iters);
        std::abort();
    }
    const double thr = -T * std::log(rng_.u01());
    if (static_cast<double>(cur_.cost) < static_cast<double>(before) + thr) {
        commit();
        ++st.accepted;
        if (cur_.cost < bestCost_) {
            bestCost_ = cur_.cost;
            best_ = cur_.seqs();
            ++st.improvements;
            shared_->offer(bestCost_, best_);
        }
    } else {
        restore();
        cur_.cost = before;
    }
}

void SisrSolver::report(double elapsed, double T, const RunStats& st, const char* phase) {
    if (!P_.verbose || tid_ != 0 || elapsed - lastReport_ < P_.reportEvery) return;
    lastReport_ = elapsed;
    std::printf("[t=%7.1fs] %s it=%lld (%.0f it/s) T=%.3f cur=%lld best=%lld global=%lld windows=%lld\n", elapsed,
                phase, st.iters, st.iters / std::max(1e-9, elapsed), T, cur_.cost, bestCost_, shared_->cost.load(),
                st.windows);
    std::fflush(stdout);
}

void SisrSolver::setSeedWindow(int tLo, int tHi) {
    auto lo = std::lower_bound(byR_.begin(), byR_.end(), tLo, [&](int f, int v) { return I_.r[f] < v; });
    auto hi = std::upper_bound(byR_.begin(), byR_.end(), tHi, [&](int v, int f) { return v < I_.r[f]; });
    seedLo_ = static_cast<int>(lo - byR_.begin());
    seedHi_ = static_cast<int>(hi - byR_.begin());
    if (seedHi_ <= seedLo_) {
        seedLo_ = 0;
        seedHi_ = I_.n;
    }
}

// Chooses the seed window. With probability P_.hotProb it covers a cluster of
// delayed flights (clusters chosen with probability ~ cost^clusterPow) plus a
// random margin; otherwise it is centred on a uniformly random flight.
void SisrSolver::pickWindow(int& tLo, int& tHi) {
    if (rng_.u01() < P_.hotProb) {
        hot_.clear();  // (start time, flight) of delayed flights
        for (const auto& R : cur_.rw)
            for (int x = 0; x < R.size(); ++x)
                if (R.S[x] > I_.r[R.seq[x]]) hot_.push_back({R.S[x], R.seq[x]});
        if (!hot_.empty()) {
            std::sort(hot_.begin(), hot_.end());
            clusters_.clear();
            double total = 0;
            for (size_t i = 0; i < hot_.size(); ++i) {
                const int f = hot_[i].second;
                const long long c = static_cast<long long>(I_.p[f]) * (hot_[i].first - I_.r[f]);
                if (i == 0 || hot_[i].first - clusters_.back().tHi > P_.clusterGap) {
                    clusters_.push_back({I_.r[f], static_cast<int>(hot_[i].first), 0.0});
                }
                Cluster& cl = clusters_.back();
                cl.tLo = std::min(cl.tLo, I_.r[f]);
                cl.tHi = std::max(cl.tHi, static_cast<int>(hot_[i].first));
                cl.weight += static_cast<double>(c);
            }
            for (auto& cl : clusters_) {
                cl.weight = std::pow(cl.weight, P_.clusterPow);
                total += cl.weight;
            }
            double u = rng_.u01() * total;
            const Cluster* pick = &clusters_.back();
            for (const auto& cl : clusters_) {
                if (u < cl.weight) {
                    pick = &cl;
                    break;
                }
                u -= cl.weight;
            }
            const int span = std::max(1, P_.marginMax - P_.marginMin + 1);
            tLo = pick->tLo - (P_.marginMin + rng_.below(span));
            tHi = pick->tHi + (P_.marginMin + rng_.below(span));
            return;
        }
    }
    const int c = rng_.below(I_.n);
    const int h = P_.wHalf / 2 + rng_.below(std::max(1, P_.wHalf));
    tLo = I_.r[c] - h;
    tHi = I_.r[c] + h;
}

RunStats SisrSolver::run(SharedBest& shared, double startTime) {
    RunStats st;
    shared_ = &shared;
    const double tl = P_.timeLimit;
    const double tGlobal = tl * P_.globalFrac;
    double elapsed = 0.0;
    shared.offer(bestCost_, best_);
    // Phase 1: global SA, temperature decays with time from T0 to Tf.
    seedLo_ = 0;
    seedHi_ = I_.n;
    const double logRatio = std::log(P_.Tf / P_.T0);
    double T = P_.T0;
    for (;;) {
        if ((st.iters & 63) == 0) {
            elapsed = nowSeconds() - startTime;
            if (elapsed >= tGlobal) break;
            T = P_.T0 * std::exp(logRatio * elapsed / tGlobal);
            report(elapsed, T, st, "global");
        }
        step(T, st);
    }
    // Phase 2: windowed SA restarts from the best solution.
    const double wLog = std::log(P_.wTf / P_.wT0);
    while (elapsed < tl) {
        if (cur_.cost != bestCost_) cur_.init(I_, best_);
        if (P_.wFixLo < P_.wFixHi) {
            setSeedWindow(P_.wFixLo, P_.wFixHi);
        } else {
            int lo = 0, hi = 0;
            pickWindow(lo, hi);
            setSeedWindow(lo, hi);
        }
        ++st.windows;
        // Per-window randomisation of the schedule (log-uniform factor in [1/spread, spread]).
        const double ls = std::log(std::max(1.0, P_.wSpread));
        const double tScale = std::exp((2.0 * rng_.u01() - 1.0) * ls);
        const double iScale = std::exp((2.0 * rng_.u01() - 1.0) * ls);
        const double wT0 = P_.wT0 * tScale;
        const long long n = std::max(1LL, static_cast<long long>(static_cast<double>(P_.wIters) * iScale));
        for (long long i = 0; i < n; ++i) {
            if ((i & 63) == 0) {
                elapsed = nowSeconds() - startTime;
                if (elapsed >= tl) break;
                report(elapsed, T, st, "window");
            }
            T = wT0 * std::exp(wLog * static_cast<double>(i) / static_cast<double>(n));
            step(T, st);
        }
    }
    if (cur_.cost != bestCost_) cur_.init(I_, best_);
    st.seconds = elapsed;
    return st;
}
