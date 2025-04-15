#include "Instance.hpp"

#ifndef VARIABLE_NEIGHBORHOOD_DESCENT_HPP
#define VARIABLE_NEIGHBORHOOD_DESCENT_HPP

class VariableNeighborhoodDescent
{
public:
    std::pair<bool, int> swapWithinRunway(
        const Instance &instance,
        std::vector<std::vector<int>> &solution,
        int currentBestCost);

    std::pair<bool, int> reinsertBetweenRunways(
        const Instance &instance,
        std::vector<std::vector<int>> &solution,
        int currentBestCost);

    std::pair<bool, int>
    swapBetweenRunways(
        const Instance &instance,
        std::vector<std::vector<int>> &solution,
        int currentBestCost);

    std::vector<std::vector<int>> vnd(const Instance &instance, std::vector<std::vector<int>> &initialSolution);
};




#endif