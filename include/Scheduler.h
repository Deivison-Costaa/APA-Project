#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include "Flight.h"
#include "Solution.h"

class Scheduler {
private:
    int n, m;
    std::vector<Flight> flights;
    std::vector<std::vector<int>> t;

    void calculateCost(Solution& sol);

public:
    Scheduler(int n, int m, const std::vector<Flight>& flights, const std::vector<std::vector<int>>& t);
    Solution greedySolve();
};

#endif