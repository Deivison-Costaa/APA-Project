#include "operators.hpp"

#include <algorithm>
#include <climits>
#include <numeric>

std::vector<int> hungarian(const std::vector<std::vector<long long>>& a) {
    const int n = (int)a.size();
    const long long INF = LLONG_MAX / 4;
    std::vector<long long> u(n + 1, 0), v(n + 1, 0);
    std::vector<int> p(n + 1, 0), way(n + 1, 0);
    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        std::vector<long long> minv(n + 1, INF);
        std::vector<char> used(n + 1, 0);
        do {
            used[j0] = 1;
            int i0 = p[j0], j1 = 0;
            long long delta = INF;
            for (int j = 1; j <= n; ++j) {
                if (used[j]) continue;
                long long cur = a[i0 - 1][j - 1] - u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }
            for (int j = 0; j <= n; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);
        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }
    std::vector<int> ans(n, 0);
    for (int j = 1; j <= n; ++j) ans[p[j] - 1] = j - 1;
    return ans;
}

namespace {

struct ChainEnd {
    int last = -1;
    int end = 0;
};

// Simulates `seq` after `from` and returns sum p (S' - base) with sync-stop against base.
long long junctionDelta(const Instance& ins, ChainEnd from, const std::vector<int>& seq, const std::vector<int>& base) {
    long long d = 0;
    int pf = from.last, end = from.end;
    for (int x : seq) {
        int s = pf < 0 ? ins.r[x] : std::max(ins.r[x], end + ins.T(pf, x));
        if (s == base[x]) break;
        d += (long long)ins.p[x] * (s - base[x]);
        end = s + ins.c[x];
        pf = x;
    }
    return d;
}

ChainEnd simulate(const Instance& ins, ChainEnd from, const std::vector<int>& seq) {
    for (int x : seq) {
        int s = from.last < 0 ? ins.r[x] : std::max(ins.r[x], from.end + ins.T(from.last, x));
        from.end = s + ins.c[x];
        from.last = x;
    }
    return from;
}

// Hungarian with random tie-breaking.
std::vector<int> noisyAssignment(std::vector<std::vector<long long>> cost, Rng& rng) {
    for (auto& row : cost)
        for (auto& x : row) x = x * 64 + rng.uniform(64);
    return hungarian(cost);
}

}  // namespace

Offspring crossoverTimeWindow(const Instance& ins, const Params& par, const Individual& A, const Individual& B,
                              Rng& rng) {
    const int n = ins.n, m = ins.m, H = ins.horizon;
    const double frac = par.winMin + rng.real() * (par.winMax - par.winMin);
    const int W = std::max(1, (int)(frac * H));
    int T1 = -W / 2 + rng.uniform(H + 1);
    if (rng.real() < par.twTargeted) {
        // centre the window on a flight scheduled differently by the two parents
        std::vector<int> diff;
        for (int x = 0; x < n; ++x)
            if (A.S[x] != B.S[x]) diff.push_back(x);
        if (!diff.empty()) T1 = B.S[diff[rng.uniform((int)diff.size())]] - W / 2;
    }
    const int T2 = T1 + W;

    std::vector<char> inMid(n, 0);
    for (int x = 0; x < n; ++x) inMid[x] = B.S[x] >= T1 && B.S[x] < T2;

    Offspring off;
    Routes pre(m), suf(m), mid(m);
    for (int i = 0; i < m; ++i)
        for (int x : A.routes[i]) {
            if (inMid[x]) continue;
            if (A.S[x] < T1) pre[i].push_back(x);
            else if (A.S[x] >= T2) suf[i].push_back(x);
            else off.missing.push_back(x);
        }
    for (int j = 0; j < m; ++j)
        for (int x : B.routes[j])
            if (inMid[x]) mid[j].push_back(x);

    std::vector<ChainEnd> preEnd(m);
    for (int i = 0; i < m; ++i) preEnd[i] = simulate(ins, ChainEnd{}, pre[i]);

    std::vector<std::vector<long long>> cost(m, std::vector<long long>(m));
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < m; ++j) cost[i][j] = junctionDelta(ins, preEnd[i], mid[j], B.S);
    std::vector<int> sigma = noisyAssignment(cost, rng);

    std::vector<ChainEnd> combEnd(m);
    for (int i = 0; i < m; ++i) combEnd[i] = simulate(ins, preEnd[i], mid[sigma[i]]);
    for (int i = 0; i < m; ++i)
        for (int k = 0; k < m; ++k) cost[i][k] = junctionDelta(ins, combEnd[i], suf[k], A.S);
    std::vector<int> tau = noisyAssignment(cost, rng);

    off.kind = 1 + sizeClass(frac, par.winMin, par.winMax);
    off.routes.assign(m, {});
    for (int i = 0; i < m; ++i) {
        auto& rw = off.routes[i];
        rw = pre[i];
        rw.insert(rw.end(), mid[sigma[i]].begin(), mid[sigma[i]].end());
        rw.insert(rw.end(), suf[tau[i]].begin(), suf[tau[i]].end());
    }
    return off;
}

Offspring crossoverRunways(const Instance& ins, const Individual& A, const Individual& B, Rng& rng) {
    const int n = ins.n, m = ins.m;
    Offspring off;
    off.kind = 9;
    if (m < 2) {
        off.routes = A.routes;
        return off;
    }
    const int k = 1 + rng.uniform(m - 1);
    std::vector<int> perm(m);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), rng);
    std::vector<char> inChild(n, 0);
    for (int i = 0; i < k; ++i) {
        off.routes.push_back(A.routes[perm[i]]);
        for (int x : A.routes[perm[i]]) inChild[x] = 1;
    }
    std::vector<std::pair<long long, int>> score(m);
    Routes filtered(m);
    for (int j = 0; j < m; ++j) {
        for (int x : B.routes[j])
            if (!inChild[x]) filtered[j].push_back(x);
        score[j] = {-(long long)filtered[j].size() * 1024 - rng.uniform(1024), j};
    }
    std::sort(score.begin(), score.end());
    for (int q = 0; q < m - k; ++q) {
        int j = score[q].second;
        for (int x : filtered[j]) inChild[x] = 1;
        off.routes.push_back(std::move(filtered[j]));
    }
    for (int x = 0; x < n; ++x)
        if (!inChild[x]) off.missing.push_back(x);
    return off;
}

Offspring ruinWindow(const Instance& ins, const Params& par, const Individual& A, Rng& rng) {
    const int n = ins.n, m = ins.m, H = ins.horizon;
    const double frac = par.ruinMin + rng.real() * (par.ruinMax - par.ruinMin);
    const int W = std::max(1, (int)(frac * H));
    // centre: a delayed flight (roulette on its delay cost) or a uniform time
    int centre = rng.uniform(H + 1);
    if (rng.real() < par.ruinTargeted && A.cost > 0) {
        long long pick = (long long)(rng.real() * (double)A.cost);
        for (int x = 0; x < n; ++x) {
            long long w = (long long)ins.p[x] * (A.S[x] - ins.r[x]);
            if (w > 0 && pick < w) {
                centre = A.S[x];
                break;
            }
            pick -= w;
        }
    }
    const int T1 = centre - W / 2;
    const int T2 = T1 + W;
    std::vector<char> hit(m, 0);
    int nHit = 0;
    for (int i = 0; i < m; ++i) nHit += hit[i] = rng.real() < par.ruinRouteProb;
    while (nHit < 2 && nHit < m) {
        int i = rng.uniform(m);
        if (!hit[i]) hit[i] = 1, ++nHit;
    }
    Offspring off;
    off.kind = 5 + sizeClass(frac, par.ruinMin, par.ruinMax);
    off.routes.assign(m, {});
    for (int i = 0; i < m; ++i)
        for (int x : A.routes[i]) {
            if (hit[i] && A.S[x] >= T1 && A.S[x] < T2) off.missing.push_back(x);
            else off.routes[i].push_back(x);
        }
    return off;
}

Routes randomGreedy(const Instance& ins, Rng& rng) {
    const int n = ins.n, m = ins.m;
    std::vector<std::pair<long long, int>> key(n);
    for (int x = 0; x < n; ++x) key[x] = {(long long)ins.r[x] + rng.uniform(40), x};
    std::sort(key.begin(), key.end());
    Routes routes(m);
    std::vector<int> last(m, -1), end(m, 0);
    for (auto [unused, x] : key) {
        (void)unused;
        long long bestKey = LLONG_MAX;
        int bestK = 0, bestS = 0;
        for (int k = 0; k < m; ++k) {
            int ready = last[k] < 0 ? 0 : end[k] + ins.T(last[k], x);
            int s = std::max(ins.r[x], ready);
            long long delayCost = (long long)ins.p[x] * (s - ins.r[x]);
            long long slack = ins.r[x] - ready;
            long long kkey = delayCost * 4096 + (slack > 0 ? std::min<long long>(slack, 4000) : 0) + rng.uniform(16);
            if (kkey < bestKey) {
                bestKey = kkey;
                bestK = k;
                bestS = s;
            }
        }
        routes[bestK].push_back(x);
        last[bestK] = x;
        end[bestK] = bestS + ins.c[x];
    }
    return routes;
}
