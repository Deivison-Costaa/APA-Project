#include <algorithm>
#include <climits>
#include "Scheduler.h"

using namespace std;

Scheduler::Scheduler(int n, int m, const vector<Flight>& flights, const vector<vector<int>>& t)
    : n(n), m(m), flights(flights), t(t) {}

void Scheduler::calculateCost(Solution& sol) {
    sol.startTimes.resize(n);
    sol.cost = 0;
    for (size_t p = 0; p < sol.allocation.size(); p++) {
        if (sol.allocation[p].empty()) continue;
        int last = sol.allocation[p][0];
        sol.startTimes[last] = flights[last].r;
        for (size_t i = 1; i < sol.allocation[p].size(); i++) {
            int curr = sol.allocation[p][i];
            sol.startTimes[curr] = max(sol.startTimes[last] + t[last][curr], flights[curr].r);
            last = curr;
        }
    }
    for (int i = 0; i < n; i++) {
        sol.cost += flights[i].p * (sol.startTimes[i] - flights[i].r);
    }
}

Solution Scheduler::greedySolve() {
    Solution sol(n, m);
    vector<pair<int, int>> order(n);
    for (int i = 0; i < n; i++) order[i] = {flights[i].r, i};
    sort(order.begin(), order.end());
    vector<int> lastTime(m, 0), lastFlight(m, -1);
    for (auto [r, i] : order) {
        int bestRunway = 0, bestStart = INT_MAX;
        for (int p = 0; p < m; p++) {
            int s = (lastFlight[p] == -1) ? r : max(lastTime[p] + t[lastFlight[p]][i], r);
            if (s < bestStart) {
                bestStart = s;
                bestRunway = p;
            }
        }
        sol.allocation[bestRunway].push_back(i);
        lastTime[bestRunway] = bestStart;
        lastFlight[bestRunway] = i;
    }
    calculateCost(sol);
    return sol;
}