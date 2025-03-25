#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>

using namespace std;

struct Solution {
    vector<vector<int>> allocation; // Flights per runway  
    vector<int> startTimes; // Start times of each flight  
    int cost; // Total cost of the solution  

    Solution(int n, int m) : allocation(m), startTimes(n), cost(0) {}
};

#endif