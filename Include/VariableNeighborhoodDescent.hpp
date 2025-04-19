#include "Instance.hpp"

#ifndef VARIABLE_NEIGHBORHOOD_DESCENT_HPP
#define VARIABLE_NEIGHBORHOOD_DESCENT_HPP

class VariableNeighborhoodDescent
{
public:
    static std::pair<bool, int> swapWithinRunway(
        const Instance &instance,
        std::vector<std::vector<int>> &solution,
        int currentBestCost);

    static std::pair<bool, int> reinsertBetweenRunways(
        const Instance &instance,
        std::vector<std::vector<int>> &solution,
        int currentBestCost);

    static std::pair<bool, int> swapBetweenRunways(
        const Instance &instance,
        std::vector<std::vector<int>> &solution,
        int currentBestCost);

    static std::pair<bool, int> reinsertWithinRunway(
        const Instance &instance,
        std::vector<std::vector<int>> &solution,
        int currentBestCost);

    void vnd(const Instance &instance, std::vector<std::vector<int>> &initialSolution);
};




#endif