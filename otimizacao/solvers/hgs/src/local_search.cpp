#include "local_search.hpp"

#include "operators.hpp"

#include <algorithm>
#include <climits>
#include <numeric>

namespace {

inline void setEdit(LocalSearch::Edit& e, int route, int a, std::initializer_list<int> L, int tr, int tp) {
    e.route = route;
    e.a = a;
    e.nL = 0;
    for (int x : L) e.L[e.nL++] = x;
    e.tr = tr;
    e.tp = tp;
}

}  // namespace

LocalSearch::LocalSearch(const Instance& ins, const Params& par)
    : I(ins), P(par), n(ins.n), m(ins.m), r(ins.r.data()), c(ins.c.data()), p(ins.p.data()), t(ins.t.data()) {
    R_.assign(m, {});
    rt_.assign(n, -1);
    pos_.assign(n, -1);
    S_.assign(n, 0);
    pred_.assign(n, -1);
    succ_.assign(n, -1);
    lastTested_.assign(n, 0);
    nodeMod_.assign(n, 0);
    order_.resize(n);
    std::iota(order_.begin(), order_.end(), 0);
    buildNeighbours();
}

void LocalSearch::buildNeighbours() {
    neigh_.assign(n, {});
    std::vector<std::pair<long long, int>> cand;
    cand.reserve(n);
    auto score = [](long long slack) { return slack >= 0 ? slack : -slack * 4; };
    for (int u = 0; u < n; ++u) {
        cand.clear();
        for (int v = 0; v < n; ++v) {
            if (v == u) continue;
            long long predSlack = (long long)r[u] - (r[v] + c[v] + I.T(v, u));
            long long succSlack = (long long)r[v] - (r[u] + c[u] + I.T(u, v));
            cand.push_back({std::min(score(predSlack), score(succSlack)), v});
        }
        int k1 = std::min<int>(P.nbCorr, (int)cand.size());
        std::partial_sort(cand.begin(), cand.begin() + k1, cand.end());
        std::vector<int>& L = neigh_[u];
        for (int i = 0; i < k1; ++i) L.push_back(cand[i].second);
        // time proximity
        cand.clear();
        for (int v = 0; v < n; ++v)
            if (v != u) cand.push_back({std::llabs((long long)r[u] - r[v]), v});
        int k2 = std::min<int>(P.nbTime + k1, (int)cand.size());
        std::partial_sort(cand.begin(), cand.begin() + k2, cand.end());
        int added = 0;
        for (int i = 0; i < k2 && added < P.nbTime; ++i) {
            int v = cand[i].second;
            if (std::find(L.begin(), L.end(), v) == L.end()) {
                L.push_back(v);
                ++added;
            }
        }
    }
}

void LocalSearch::refreshRoute(int k, int from, bool mark) {
    const std::vector<int>& A = R_[k];
    const int len = (int)A.size();
    from = std::max(0, std::min(from, len));
    int prev = from > 0 ? A[from - 1] : -1;
    int end = prev >= 0 ? S_[prev] + c[prev] : 0;
    for (int q = from; q < len; ++q) {
        int x = A[q];
        int s = prev < 0 ? r[x] : std::max(r[x], end + t[(size_t)prev * n + x]);
        int nx = q + 1 < len ? A[q + 1] : -1;
        if (mark && (s != S_[x] || pred_[x] != prev || succ_[x] != nx)) nodeMod_[x] = stamp_;
        S_[x] = s;
        pred_[x] = prev;
        succ_[x] = nx;
        rt_[x] = k;
        pos_[x] = q;
        end = s + c[x];
        prev = x;
    }
}

long long LocalSearch::windowMod(int x) const {
    long long w = nodeMod_[x];
    int a = pred_[x];
    if (a >= 0) w = std::max(w, nodeMod_[a]);
    int b = succ_[x];
    if (b >= 0) {
        w = std::max(w, nodeMod_[b]);
        int b2 = succ_[b];
        if (b2 >= 0) w = std::max(w, nodeMod_[b2]);
    }
    return w;
}

void LocalSearch::load(const Routes& routes) {
    R_.assign(m, {});
    for (int k = 0; k < m && k < (int)routes.size(); ++k) R_[k] = routes[k];
    std::fill(rt_.begin(), rt_.end(), -1);
    std::fill(pos_.begin(), pos_.end(), -1);
    std::fill(pred_.begin(), pred_.end(), -1);
    std::fill(succ_.begin(), succ_.end(), -1);
    for (int x = 0; x < n; ++x) S_[x] = r[x];
    total_ = 0;
    ++version_;
    for (int k = 0; k < m; ++k) {
        refreshRoute(k, 0, false);
        for (int x : R_[k]) total_ += (long long)p[x] * (S_[x] - r[x]);
    }
}

long long LocalSearch::evalEdit(const Edit& e) {
    ++nbEvals_;
    long long d = 0;
    int pf = e.a >= 0 ? R_[e.route][e.a] : -1;
    int end = pf >= 0 ? S_[pf] + c[pf] : 0;
    for (int k = 0; k < e.nL; ++k) {
        int x = e.L[k];
        int s = pf >= 0 ? std::max(r[x], end + t[(size_t)pf * n + x]) : r[x];
        d += (long long)p[x] * (s - S_[x]);
        end = s + c[x];
        pf = x;
    }
    const std::vector<int>& T = R_[e.tr];
    const int len = (int)T.size();
    for (int q = e.tp; q < len; ++q) {
        int x = T[q];
        int s = pf >= 0 ? std::max(r[x], end + t[(size_t)pf * n + x]) : r[x];
        if (s == S_[x]) break;
        d += (long long)p[x] * (s - S_[x]);
        end = s + c[x];
        pf = x;
    }
    return d;
}

void LocalSearch::applyEdits(const Edit* e, int ne) {
    if ((int)nvBuf_.size() < ne) nvBuf_.resize(ne);
    std::vector<std::vector<int>>& nv = nvBuf_;
    for (int i = 0; i < ne; ++i) {
        const std::vector<int>& A = R_[e[i].route];
        nv[i].assign(A.begin(), A.begin() + (e[i].a + 1));
        nv[i].insert(nv[i].end(), e[i].L, e[i].L + e[i].nL);
        const std::vector<int>& T = R_[e[i].tr];
        if (e[i].tp < (int)T.size()) nv[i].insert(nv[i].end(), T.begin() + e[i].tp, T.end());
    }
    ++nbMoves_;
    ++stamp_;
    ++version_;
    for (int i = 0; i < ne; ++i) R_[e[i].route].swap(nv[i]);
    for (int i = 0; i < ne; ++i) refreshRoute(e[i].route, e[i].a, true);
}

bool LocalSearch::commitIfBetter(Edit* e, int ne) {
    long long d = evalEdit(e[0]);
    for (int i = 1; i < ne; ++i) d += evalEdit(e[i]);
    if (d >= 0) return false;
    applyEdits(e, ne);
    total_ += d;
    return true;
}

bool LocalSearch::commitWithKnown(long long d0, Edit* e, int ne) {
    long long d = d0;
    for (int i = 1; i < ne; ++i) d += evalEdit(e[i]);
    if (d >= 0) return false;
    applyEdits(e, ne);
    total_ += d;
    return true;
}

long long LocalSearch::removalDelta(int u, int len) {
    if (remFor_ != u || remStamp_ != version_) {
        remFor_ = u;
        remStamp_ = version_;
        rem_[1] = rem_[2] = rem_[3] = LLONG_MIN;
    }
    if (rem_[len] == LLONG_MIN) {
        Edit e;
        setEdit(e, rt_[u], pos_[u] - 1, {}, rt_[u], pos_[u] + len);
        rem_[len] = evalEdit(e);
    }
    return rem_[len];
}

void LocalSearch::insertMissing(const std::vector<int>& flights) {
    Edit e;
    for (int x : flights) {
        S_[x] = r[x];  // baseline: an unassigned flight contributes nothing
        long long best = LLONG_MAX;
        Edit bestE{};
        for (int k = 0; k < m; ++k) {
            const std::vector<int>& A = R_[k];
            int len = (int)A.size();
            // first position whose start is >= r[x]
            int lo = 0, hi = len;
            while (lo < hi) {
                int mid = (lo + hi) / 2;
                if (S_[A[mid]] < r[x]) lo = mid + 1; else hi = mid;
            }
            int from = std::max(0, lo - 3), to = std::min(len, lo + 2);
            for (int q = from; q <= to; ++q) {
                setEdit(e, k, q - 1, {x}, k, q);
                long long d = evalEdit(e);
                if (d < best) {
                    best = d;
                    bestE = e;
                }
            }
        }
        applyEdits(&bestE, 1);
        total_ += best;
    }
}

bool LocalSearch::tryInter(int u, int v) {
    const int U = rt_[u], V = rt_[v];
    const std::vector<int>& A = R_[U];
    const std::vector<int>& B = R_[V];
    const int iu = pos_[u], iv = pos_[v];
    const int lenA = (int)A.size(), lenB = (int)B.size();
    Edit e[2];
    // relocate u after v / before v
    const long long rem1 = removalDelta(u, 1);
    setEdit(e[0], U, iu - 1, {}, U, iu + 1);
    setEdit(e[1], V, iv, {u}, V, iv + 1);
    if (commitWithKnown(rem1, e, 2)) return true;
    setEdit(e[1], V, iv - 1, {u}, V, iv);
    if (commitWithKnown(rem1, e, 2)) return true;
    // swap u <-> v
    setEdit(e[0], U, iu - 1, {v}, U, iu + 1);
    setEdit(e[1], V, iv - 1, {u}, V, iv + 1);
    if (commitIfBetter(e, 2)) return true;
    // 2-opt*: v becomes predecessor of u
    setEdit(e[0], U, iu - 1, {}, V, iv + 1);
    setEdit(e[1], V, iv, {}, U, iu);
    if (commitIfBetter(e, 2)) return true;
    // 2-opt*: u becomes predecessor of v
    setEdit(e[0], U, iu, {}, V, iv);
    setEdit(e[1], V, iv - 1, {}, U, iu + 1);
    if (commitIfBetter(e, 2)) return true;
    if (iv + 1 < lenB) {
        const int v2 = B[iv + 1];
        // swap u <-> (v, v2)
        setEdit(e[0], U, iu - 1, {v, v2}, U, iu + 1);
        setEdit(e[1], V, iv - 1, {u}, V, iv + 2);
        if (commitIfBetter(e, 2)) return true;
    }
    if (iu + 1 < lenA) {
        const int u2 = A[iu + 1];
        // relocate (u, u2) after v / before v
        const long long rem2 = removalDelta(u, 2);
        setEdit(e[0], U, iu - 1, {}, U, iu + 2);
        setEdit(e[1], V, iv, {u, u2}, V, iv + 1);
        if (commitWithKnown(rem2, e, 2)) return true;
        setEdit(e[1], V, iv - 1, {u, u2}, V, iv);
        if (commitWithKnown(rem2, e, 2)) return true;
        // swap (u, u2) <-> v
        setEdit(e[0], U, iu - 1, {v}, U, iu + 2);
        setEdit(e[1], V, iv - 1, {u, u2}, V, iv + 1);
        if (commitIfBetter(e, 2)) return true;
        if (iv + 1 < lenB) {
            const int v2 = B[iv + 1];
            setEdit(e[0], U, iu - 1, {v, v2}, U, iu + 2);
            setEdit(e[1], V, iv - 1, {u, u2}, V, iv + 2);
            if (commitIfBetter(e, 2)) return true;
        }
        if (iu + 2 < lenA) {
            const int u3 = A[iu + 2];
            const long long rem3 = removalDelta(u, 3);
            setEdit(e[0], U, iu - 1, {}, U, iu + 3);
            setEdit(e[1], V, iv, {u, u2, u3}, V, iv + 1);
            if (commitWithKnown(rem3, e, 2)) return true;
            setEdit(e[1], V, iv - 1, {u, u2, u3}, V, iv);
            if (commitWithKnown(rem3, e, 2)) return true;
        }
    }
    return P.ejection && tryEjection(u, v);
}

// Ejection chain of depth 2: u takes the place of w (w = v or succ(v)) on runway V, and w
// is inserted next to one of its neighbours on a third runway.
bool LocalSearch::tryEjection(int u, int v) {
    const int U = rt_[u], V = rt_[v];
    const int iu = pos_[u];
    Edit e[3];
    for (int variant = 0; variant < 2; ++variant) {
        const int w = variant == 0 ? v : succ_[v];
        if (w < 0) continue;
        const int iw = pos_[w];
        setEdit(e[0], U, iu - 1, {}, U, iu + 1);
        setEdit(e[1], V, iw - 1, {u}, V, iw + 1);
        const long long d0 = evalEdit(e[0]) + evalEdit(e[1]);
        if (d0 + (long long)p[w] * (r[w] - S_[w]) >= 0) continue;
        for (int z : neigh_[w]) {
            const int X = rt_[z];
            if (X == U || X == V) continue;
            const int iz = pos_[z];
            setEdit(e[2], X, iz, {w}, X, iz + 1);
            long long d = d0 + evalEdit(e[2]);
            if (d >= 0) {
                setEdit(e[2], X, iz - 1, {w}, X, iz);
                d = d0 + evalEdit(e[2]);
            }
            if (d < 0) {
                applyEdits(e, 3);
                total_ += d;
                return true;
            }
        }
    }
    return false;
}

bool LocalSearch::tryIntra(int u, int v) {
    const int U = rt_[u];
    const std::vector<int>& A = R_[U];
    const int iu = pos_[u], iv = pos_[v];
    const int lenA = (int)A.size();
    const int W = std::min(P.maxIntraWin, MAXL);
    if (std::abs(iu - iv) + 2 > W) return false;
    Edit e;
    auto window = [&](int lo, int hi) {
        e.route = U;
        e.a = lo - 1;
        e.tr = U;
        e.tp = hi + 1;
        e.nL = 0;
    };
    auto push = [&](int x) { e.L[e.nL++] = x; };
    auto commit = [&]() { return commitIfBetter(&e, 1); };
    // relocate u after v
    if (iv < iu - 1) {
        window(iv + 1, iu);
        push(u);
        for (int q = iv + 1; q < iu; ++q) push(A[q]);
        if (commit()) return true;
    } else if (iv > iu) {
        window(iu, iv);
        for (int q = iu + 1; q <= iv; ++q) push(A[q]);
        push(u);
        if (commit()) return true;
    }
    // relocate u before v
    if (iv < iu) {
        window(iv, iu);
        push(u);
        for (int q = iv; q < iu; ++q) push(A[q]);
        if (commit()) return true;
    } else if (iv > iu + 1) {
        window(iu, iv - 1);
        for (int q = iu + 1; q < iv; ++q) push(A[q]);
        push(u);
        if (commit()) return true;
    }
    // swap u <-> v
    {
        int lo = std::min(iu, iv), hi = std::max(iu, iv);
        window(lo, hi);
        for (int q = lo; q <= hi; ++q) push(q == lo ? A[hi] : q == hi ? A[lo] : A[q]);
        if (commit()) return true;
    }
    // relocate (u, u2) after v
    if (iu + 1 < lenA) {
        const int u2 = A[iu + 1];
        if (iv < iu - 1) {
            window(iv + 1, iu + 1);
            push(u);
            push(u2);
            for (int q = iv + 1; q < iu; ++q) push(A[q]);
            if (commit()) return true;
        } else if (iv > iu + 1) {
            window(iu, iv);
            for (int q = iu + 2; q <= iv; ++q) push(A[q]);
            push(u);
            push(u2);
            if (commit()) return true;
        }
    }
    return false;
}

bool LocalSearch::tryEmpty(int u) {
    const int U = rt_[u];
    const int iu = pos_[u];
    const int lenA = (int)R_[U].size();
    Edit e[2];
    for (int k = 0; k < m; ++k) {
        if (!R_[k].empty() || k == U) continue;
        setEdit(e[0], U, iu - 1, {}, U, iu + 1);
        setEdit(e[1], k, -1, {u}, k, 0);
        if (commitIfBetter(e, 2)) return true;
        setEdit(e[0], U, iu - 1, {}, U, lenA);
        setEdit(e[1], k, -1, {}, U, iu);
        if (iu > 0 && commitIfBetter(e, 2)) return true;
    }
    return false;
}

bool LocalSearch::tryPair(int u, int v) {
    return rt_[u] == rt_[v] ? tryIntra(u, v) : tryInter(u, v);
}

long long LocalSearch::run(Rng& rng, const std::vector<char>* dirty) {
    stamp_ = 1;
    for (int x = 0; x < n; ++x) {
        lastTested_[x] = 0;
        nodeMod_[x] = (!dirty || (*dirty)[x]) ? 1 : 0;
    }
    for (auto& L : neigh_)
        if (rng.uniform(4) == 0) std::shuffle(L.begin(), L.end(), rng);
    bool improved = true;
    while (improved) {
        improved = false;
        std::shuffle(order_.begin(), order_.end(), rng);
        bool anyEmpty = false;
        for (int k = 0; k < m; ++k) anyEmpty |= R_[k].empty();
        for (int u : order_) {
            if (rt_[u] < 0) continue;
            const long long lastT = lastTested_[u];
            lastTested_[u] = stamp_;
            long long wu = windowMod(u);
            for (int v : neigh_[u]) {
                if (wu <= lastT && windowMod(v) <= lastT) continue;
                if (tryPair(u, v)) {
                    improved = true;
                    wu = windowMod(u);
                }
            }
            if (anyEmpty && wu > lastT && tryEmpty(u)) improved = true;
        }
    }
    return total_;
}

bool LocalSearch::cutPass() {
    std::vector<int> times;
    for (int k = 0; k < m; ++k)
        for (int x : R_[k]) times.push_back(S_[x]);
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());
    std::vector<int> q(m);
    std::vector<std::vector<long long>> cost(m, std::vector<long long>(m, 0));
    std::vector<Edit> edits(m);
    bool any = false;
    for (int T : times) {
        for (int k = 0; k < m; ++k) {
            const std::vector<int>& A = R_[k];
            q[k] = (int)(std::lower_bound(A.begin(), A.end(), T, [&](int x, int tt) { return S_[x] < tt; }) -
                         A.begin());
        }
        Edit e;
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < m; ++j) {
                if (i == j) {
                    cost[i][j] = 0;
                    continue;
                }
                setEdit(e, i, q[i] - 1, {}, j, q[j]);
                cost[i][j] = evalEdit(e);
            }
        std::vector<int> sigma = hungarian(cost);
        long long d = 0;
        int ne = 0;
        for (int i = 0; i < m; ++i) {
            d += cost[i][sigma[i]];
            if (sigma[i] != i) setEdit(edits[ne++], i, q[i] - 1, {}, sigma[i], q[sigma[i]]);
        }
        if (d < 0) {
            applyEdits(edits.data(), ne);
            total_ += d;
            any = true;
        }
    }
    return any;
}

bool LocalSearch::consistent() const {
    std::vector<int> S2;
    long long tot = evaluateRoutes(I, R_, &S2);
    if (tot != total_) return false;
    for (int k = 0; k < m; ++k)
        for (int q = 0; q < (int)R_[k].size(); ++q) {
            int x = R_[k][q];
            if (rt_[x] != k || pos_[x] != q || S_[x] != S2[x]) return false;
        }
    return true;
}
