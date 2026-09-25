#include "construct.hpp"

#include <algorithm>
#include <limits>

Solution constructGreedy(const Instance& ins, Rng& rng, double noise) {
    std::vector<std::vector<int>> runways(ins.m);
    std::vector<int> endTime(ins.m, 0), last(ins.m, -1);
    for (int f : ins.byRelease) {
        int bestK = 0;
        double bestScore = std::numeric_limits<double>::max();
        int bestStart = 0;
        for (int k = 0; k < ins.m; ++k) {
            const int s = last[k] < 0 ? ins.r[f] : std::max(ins.r[f], endTime[k] + ins.sep(last[k], f));
            const long long delay = static_cast<long long>(ins.p[f]) * (s - ins.r[f]);
            // idle time before f (smaller = tighter fit); empty runways are a last resort
            const int idle = last[k] < 0 ? 100000 : ins.r[f] - (endTime[k] + ins.sep(last[k], f));
            double score = static_cast<double>(delay) * 1000.0 + std::max(idle, 0);
            if (noise > 0) score += noise * rng.uniform() * 1000.0;
            if (score < bestScore) {
                bestScore = score;
                bestK = k;
                bestStart = s;
            }
        }
        runways[bestK].push_back(f);
        endTime[bestK] = bestStart + ins.c[f];
        last[bestK] = f;
    }
    return solutionFromRunways(ins, runways);
}
