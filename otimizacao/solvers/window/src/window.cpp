#include "window.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <queue>
#include <unordered_map>

#include "hungarian.hpp"
#include "mip.hpp"

namespace {
struct Src {
    int rw, lo, prev;
    long long fin;
};
struct Snk {
    int rw, hi, head;
};
struct StateRec {
    int f, d;
    long long lab;  // min reduced cost of a partial path from a source to this state
};
struct Arc {
    int tail, head;  // nodes: [0,R) sources, [R,2R) sinks, 2R+s states
    long long cost, rc;
};
constexpr long long kBig = 1LL << 45;

// Cost of runway suffix (chain z) when its head starts at T (T >= r_head), with sync-stop.
long long chainCost(const Solution& s, const Snk& z, long long T) {
    if (z.head < 0) return 0;
    const Instance& I = *s.I;
    const auto& sq = s.seq[z.rw];
    const auto& S = s.st[z.rw];
    const auto& C = s.cum[z.rw];
    const int L = static_cast<int>(sq.size());
    long long cost = 0, fin = 0;
    int prev = -1;
    for (int q = z.hi; q < L; ++q) {
        const int y = sq[q];
        const long long sy = (q == z.hi) ? T : std::max<long long>(I.r[y], fin + I.T(prev, y));
        if (sy == S[q]) return cost + C[L] - C[q];
        cost += I.p[y] * (sy - I.r[y]);
        fin = sy + I.c[y];
        prev = y;
    }
    return cost;
}

double secondsSince(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}
}  // namespace

long long WindowOpt::optimize(Solution& s, int T0, int T1, const std::vector<int>& runways, double timeLimit,
                              WindowStats& stats, std::vector<int>* touched, int maxFree) {
    const auto tStart = std::chrono::steady_clock::now();
    const int R = static_cast<int>(runways.size());
    std::vector<Src> src(R);
    std::vector<Snk> snk(R);
    std::vector<int> F, incDelay;
    long long ubw = 0;
    for (int idx = 0; idx < R; ++idx) {
        const int k = runways[idx];
        const auto& S = s.st[k];
        const int L = s.len(k);
        const int lo = static_cast<int>(std::lower_bound(S.begin(), S.end(), T0) - S.begin());
        const int hi = static_cast<int>(std::lower_bound(S.begin(), S.end(), T1) - S.begin());
        const int prev = lo > 0 ? s.seq[k][lo - 1] : -1;
        src[idx] = {k, lo, prev, lo > 0 ? S[lo - 1] + I.c[prev] : 0LL};
        snk[idx] = {k, hi, hi < L ? s.seq[k][hi] : -1};
        for (int q = lo; q < hi; ++q) {
            F.push_back(s.seq[k][q]);
            incDelay.push_back(S[q] - I.r[s.seq[k][q]]);
        }
        ubw += s.cum[k][L] - s.cum[k][lo];
    }
    const int nF = static_cast<int>(F.size());
    if (nF > maxFree || (nF == 0 && R < 2)) {
        ++stats.skipped;
        return 0;
    }
    ++stats.calls;
    std::vector<long long> fmin(R, 0);
    long long sumMin = 0;
    for (int l = 0; l < R; ++l) {
        fmin[l] = snk[l].head < 0 ? 0 : chainCost(s, snk[l], I.r[snk[l].head]);
        sumMin += fmin[l];
    }

    // ---- assignment relaxation: tails (R sources + nF flights) x heads (nF flights + R sinks)
    auto tailOf = [&](int row, int& prevG, long long& fin) {
        if (row < R) {
            prevG = src[row].prev;
            fin = src[row].fin;
        } else {
            prevG = F[row - R];
            fin = static_cast<long long>(I.r[prevG]) + I.c[prevG];
        }
    };
    auto headCost = [&](int prevG, long long fin, int col) -> long long {  // extra cost of head col
        if (col < nF) {
            const int g = F[col];
            if (prevG < 0) return 0;
            return I.p[g] * std::max<long long>(0, fin + I.T(prevG, g) - I.r[g]);
        }
        const Snk& z = snk[col - nF];
        if (z.head < 0) return 0;
        long long T = I.r[z.head];
        if (prevG >= 0) T = std::max<long long>(T, fin + I.T(prevG, z.head));
        return chainCost(s, z, T) - fmin[col - nF];
    };
    const int N = R + nF;
    std::vector<long long> amat(static_cast<size_t>(N) * N, kBig);
    for (int row = 0; row < N; ++row) {
        int prevG;
        long long fin;
        tailOf(row, prevG, fin);
        for (int col = 0; col < N; ++col) {
            if (row >= R && col == row - R) continue;
            amat[static_cast<size_t>(row) * N + col] = headCost(prevG, fin, col);
        }
    }
    const AssignResult asg = hungarian(amat, N);
    const long long gap = ubw - sumMin - asg.cost;
    stats.lbTime += secondsSince(tStart);
    if (gap < 0) {
        std::fprintf(stderr, "window: negative gap %lld (bug)\n", gap);
        return 0;
    }
    if (gap == 0) {  // incumbent proven optimal for this window by the assignment bound
        ++stats.provenByLb;
        return 0;
    }

    // ---- forward generation of time-indexed states with reduced-cost filtering
    std::vector<std::vector<int>> sid(nF);
    for (int f = 0; f < nF; ++f) sid[f].assign(std::max(dcap_, incDelay[f]) + 1, -1);
    std::vector<StateRec> states;
    std::vector<Arc> arcs;
    std::vector<int> arcBegin;  // per tail node (sources first, then states in expansion order)
    using HeapItem = std::pair<long long, int>;
    std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> heap;
    auto expand = [&](int tailNode, int row, long long lab, int tailLocal) {
        int prevG;
        long long fin;
        if (row < R) {
            prevG = src[row].prev;
            fin = src[row].fin;
        } else {
            prevG = F[tailLocal];
            fin = static_cast<long long>(I.r[prevG]) + states[tailNode - 2 * R].d + I.c[prevG];
        }
        const long long ur = asg.u[row];
        for (int f = 0; f < nF; ++f) {
            if (f == tailLocal) continue;
            const int g = F[f];
            long long sp = I.r[g];
            if (prevG >= 0) sp = std::max<long long>(sp, fin + I.T(prevG, g));
            const long long d = sp - I.r[g];
            if (d >= static_cast<long long>(sid[f].size())) continue;
            const long long cost = I.p[g] * d;
            const long long rc = cost - ur - asg.v[f];
            if (lab + rc > gap) continue;
            int& id = sid[f][d];
            if (id < 0) {
                id = static_cast<int>(states.size());
                states.push_back({f, static_cast<int>(d), lab + rc});
                heap.push({sp, id});
            } else if (lab + rc < states[id].lab) {
                states[id].lab = lab + rc;
            }
            arcs.push_back({tailNode, 2 * R + id, cost, rc});
        }
        for (int l = 0; l < R; ++l) {
            const long long extra = headCost(prevG, fin, nF + l);
            const long long rc = extra - ur - asg.v[nF + l];
            if (lab + rc > gap) continue;
            arcs.push_back({tailNode, R + l, extra + fmin[l], rc});
        }
    };
    std::vector<int> tailStart(2 * R, 0), tailEnd(2 * R, 0);
    for (int k = 0; k < R; ++k) {
        tailStart[k] = static_cast<int>(arcs.size());
        expand(k, k, 0, -1);
        tailEnd[k] = static_cast<int>(arcs.size());
    }
    std::vector<int> stStart, stEnd, order;
    while (!heap.empty()) {
        const int id = heap.top().second;
        heap.pop();
        if (static_cast<int>(stStart.size()) < static_cast<int>(states.size())) {
            stStart.resize(states.size(), 0);
            stEnd.resize(states.size(), 0);
        }
        stStart[id] = static_cast<int>(arcs.size());
        expand(2 * R + id, R + states[id].f, states[id].lab, states[id].f);
        stEnd.resize(states.size(), 0);
        stStart.resize(states.size(), 0);
        stEnd[id] = static_cast<int>(arcs.size());
        order.push_back(id);
    }
    const int nS0 = static_cast<int>(states.size());
    // ---- backward pass: min reduced cost from each state to a sink
    std::vector<long long> outRc(nS0, kBig);
    auto headOut = [&](int head) { return head < 2 * R ? 0LL : outRc[head - 2 * R]; };
    for (int q = static_cast<int>(order.size()) - 1; q >= 0; --q) {
        const int id = order[q];
        long long best = kBig;
        for (int a = stStart[id]; a < stEnd[id]; ++a) best = std::min(best, arcs[a].rc + headOut(arcs[a].head));
        outRc[id] = best;
    }
    auto tailLab = [&](int tail) { return tail < R ? 0LL : states[tail - 2 * R].lab; };
    std::vector<char> keepArc(arcs.size(), 0);
    std::vector<int> newId(nS0, -1);
    int nS = 0;
    for (size_t a = 0; a < arcs.size(); ++a) {
        const Arc& ar = arcs[a];
        if (tailLab(ar.tail) + ar.rc + headOut(ar.head) > gap) continue;
        keepArc[a] = 1;
        if (ar.head >= 2 * R && newId[ar.head - 2 * R] < 0) newId[ar.head - 2 * R] = nS++;
    }
    std::vector<Arc> kept;
    kept.reserve(arcs.size());
    auto remap = [&](int node) { return node < 2 * R ? node : 2 * R + newId[node - 2 * R]; };
    for (size_t a = 0; a < arcs.size(); ++a) {
        if (!keepArc[a]) continue;
        const Arc& ar = arcs[a];
        if (ar.tail >= 2 * R && newId[ar.tail - 2 * R] < 0) continue;  // unreachable tail
        kept.push_back({remap(ar.tail), remap(ar.head), ar.cost, ar.rc});
    }
    std::vector<int> stateFlight(nS, -1);
    for (int id = 0; id < nS0; ++id)
        if (newId[id] >= 0) stateFlight[newId[id]] = states[id].f;
    const int nA = static_cast<int>(kept.size());
    stats.sumFree += nF;
    stats.sumStates += nS;
    stats.sumArcs += nA;

    // ---- model
    BinModel md;
    md.numRows = 2 * R + nF + nS;
    md.rowLo.assign(md.numRows, 1.0);
    md.rowUp.assign(md.numRows, 1.0);
    for (int q = 2 * R + nF; q < md.numRows; ++q) md.rowLo[q] = md.rowUp[q] = 0.0;
    md.cost.reserve(nA);
    md.start.reserve(nA + 1);
    md.start.push_back(0);
    for (const Arc& a : kept) {
        md.cost.push_back(static_cast<double>(a.cost));
        if (a.tail < R) {
            md.index.push_back(a.tail);
            md.value.push_back(1.0);
        } else {
            md.index.push_back(2 * R + nF + (a.tail - 2 * R));
            md.value.push_back(-1.0);
        }
        if (a.head < 2 * R) {
            md.index.push_back(a.head);
            md.value.push_back(1.0);
        } else {
            const int sIdx = a.head - 2 * R;
            md.index.push_back(2 * R + stateFlight[sIdx]);
            md.value.push_back(1.0);
            md.index.push_back(2 * R + nF + sIdx);
            md.value.push_back(1.0);
        }
        md.start.push_back(static_cast<int>(md.index.size()));
    }
    // ---- incumbent as initial solution
    const long long NN = 2LL * R + nS;
    std::unordered_map<long long, int> arcOf;
    arcOf.reserve(nA * 2);
    for (int a = 0; a < nA; ++a) arcOf.emplace(kept[a].tail * NN + kept[a].head, a);
    std::vector<double> init(nA, 0.0);
    {
        int fpos = 0;
        bool ok = true;
        for (int idx = 0; idx < R && ok; ++idx) {
            int tail = idx;
            for (int q = src[idx].lo; q < snk[idx].hi; ++q, ++fpos) {
                const int old = sid[fpos][incDelay[fpos]];
                if (old < 0 || newId[old] < 0) { ok = false; break; }
                const int head = 2 * R + newId[old];
                auto it = arcOf.find(tail * NN + head);
                if (it == arcOf.end()) { ok = false; break; }
                init[it->second] = 1.0;
                tail = head;
            }
            if (!ok) break;
            auto it = arcOf.find(tail * NN + R + idx);
            if (it == arcOf.end()) { ok = false; break; }
            init[it->second] = 1.0;
        }
        if (!ok) {
            std::fprintf(stderr, "window: incumbent not representable (bug)\n");
            return 0;
        }
    }
    const auto t0 = std::chrono::steady_clock::now();
    const double tl = std::max(0.01, timeLimit - secondsSince(tStart));
    MipResult res = solveBinary(md, &init, tl);
    stats.mipTime += secondsSince(t0);
    if (res.optimal) ++stats.optimal;
    else ++stats.timeouts;
    if (stats.verbose > 1)
        std::printf("    win T0=%d T1=%d R=%d F=%d S=%d/%d A=%d ubw=%lld gap=%lld obj=%.0f bound=%.1f nodes=%lld "
                    "opt=%d t=%.3f\n",
                    T0, T1, R, nF, nS, nS0, nA, ubw, gap, res.objective, res.bound, res.nodes,
                    static_cast<int>(res.optimal), secondsSince(tStart));
    if (!res.feasible) return 0;
    const long long newObj = std::llround(res.objective);
    if (newObj >= ubw) return 0;

    // ---- extract
    std::vector<int> nxt(2 * R + nS, -1);
    for (int a = 0; a < nA; ++a)
        if (res.x[a] > 0.5) nxt[kept[a].tail] = kept[a].head;
    std::vector<std::vector<int>> newSeq(R), oldSeq(R);
    std::vector<char> used(nF, 0);
    for (int idx = 0; idx < R; ++idx) {
        const int k = runways[idx];
        std::vector<int> ns(s.seq[k].begin(), s.seq[k].begin() + src[idx].lo);
        int node = nxt[idx];
        int guard = 0;
        while (node >= 2 * R && guard++ <= nF) {
            const int f = stateFlight[node - 2 * R];
            if (used[f]) { node = -1; break; }
            used[f] = 1;
            ns.push_back(F[f]);
            node = nxt[node];
        }
        if (node < R || node >= 2 * R) {
            std::fprintf(stderr, "window: bad MIP path (bug)\n");
            return 0;
        }
        const Snk& z = snk[node - R];
        ns.insert(ns.end(), s.seq[z.rw].begin() + z.hi, s.seq[z.rw].end());
        newSeq[idx] = std::move(ns);
    }
    const long long before = s.cost;
    for (int idx = 0; idx < R; ++idx) oldSeq[idx] = s.seq[runways[idx]];
    for (int idx = 0; idx < R; ++idx) s.setRunway(runways[idx], std::move(newSeq[idx]), src[idx].lo);
    const long long expect = before - ubw + newObj;
    if (s.cost != expect) {
        std::fprintf(stderr, "window: cost mismatch expected %lld got %lld (reverting)\n", expect, s.cost);
        for (int idx = 0; idx < R; ++idx) s.setRunway(runways[idx], std::move(oldSeq[idx]), 0);
        return 0;
    }
    ++stats.improved;
    stats.gain += before - s.cost;
    if (touched) {
        for (int g : F) touched->push_back(g);
        for (int idx = 0; idx < R; ++idx) {
            if (src[idx].prev >= 0) touched->push_back(src[idx].prev);
            if (snk[idx].head >= 0) touched->push_back(snk[idx].head);
        }
    }
    return s.cost - before;
}
