#include "Instance.hpp"

#ifndef GREEDYALGORITHM_HPP
#define GREEDYALGORITHM_HPP

class GreedyAlgorithm{
public:
    std::vector<std::vector<int>> nearestNeighbor(const Instance& instance);
    std::vector<std::vector<int>> cheaperInsertion(const Instance& Instance);
};





#endif