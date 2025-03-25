#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>

struct Solution {
    std::vector<std::vector<int>> allocation; // Flights per runway
    std::vector<int> startTimes;              // Start times of each flight 
    int cost;                                 // Total cost of the solution 

    Solution(int n, int m) : allocation(m), startTimes(n), cost(0) {}
};

#endif