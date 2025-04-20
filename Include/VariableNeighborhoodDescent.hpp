#ifndef VARIABLE_NEIGHBORHOOD_DESCENT_HPP
#define VARIABLE_NEIGHBORHOOD_DESCENT_HPP
#include "Instance.hpp"

class VariableNeighborhoodDescent
{
private:
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
    
public:
    void vnd(const Instance &instance, std::vector<std::vector<int>> &initialSolution);
};




#endif