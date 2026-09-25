#include "hungarian.hpp"

#include <limits>

AssignResult hungarian(const std::vector<long long>& a, int N) {
    // classic potentials-based implementation (1-indexed internally)
    const long long INF = std::numeric_limits<long long>::max() / 4;
    std::vector<long long> u(N + 1, 0), v(N + 1, 0), minv(N + 1);
    std::vector<int> p(N + 1, 0), way(N + 1, 0);
    std::vector<char> used(N + 1);
    for (int i = 1; i <= N; ++i) {
        p[0] = i;
        int j0 = 0;
        std::fill(minv.begin(), minv.end(), INF);
        std::fill(used.begin(), used.end(), 0);
        do {
            used[j0] = 1;
            const int i0 = p[j0];
            long long delta = INF;
            int j1 = 0;
            const long long* row = a.data() + static_cast<size_t>(i0 - 1) * N;
            for (int j = 1; j <= N; ++j)
                if (!used[j]) {
                    const long long cur = row[j - 1] - u[i0] - v[j];
                    if (cur < minv[j]) {
                        minv[j] = cur;
                        way[j] = j0;
                    }
                    if (minv[j] < delta) {
                        delta = minv[j];
                        j1 = j;
                    }
                }
            for (int j = 0; j <= N; ++j)
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            j0 = j1;
        } while (p[j0] != 0);
        do {
            const int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }
    AssignResult res;
    res.colOfRow.assign(N, -1);
    for (int j = 1; j <= N; ++j)
        if (p[j]) res.colOfRow[p[j] - 1] = j - 1;
    res.u.assign(u.begin() + 1, u.end());
    res.v.assign(v.begin() + 1, v.end());
    for (int i = 0; i < N; ++i) res.cost += a[static_cast<size_t>(i) * N + res.colOfRow[i]];
    return res;
}
