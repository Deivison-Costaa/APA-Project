#include "Instance.hpp"

#ifndef VARIABLE_NEIGHBORHOOD_DESCENT_HPP
#define VARIABLE_NEIGHBORHOOD_DESCENT_HPP

class VariableNeighborhoodDescent
{
public:
    std::pair<std::vector<std::vector<int>>, int> swapWithinRunway(
        const Instance &instance,
        const std::vector<std::vector<int>> &solution,
        int currentBestCost);

    std::pair<std::vector<std::vector<int>>, int> reinsertBetweenRunways(
        const Instance &instance,
        const std::vector<std::vector<int>> &solution,
        int currentBestCost);

    std::pair<std::vector<std::vector<int>>, int> swapBetweenRunways(
        const Instance &instance,
        const std::vector<std::vector<int>> &solution,
        int currentBestCost);

    std::vector<std::vector<int>> vnd(const Instance &instance, const std::vector<std::vector<int>> &initialSolution);
};




#endif