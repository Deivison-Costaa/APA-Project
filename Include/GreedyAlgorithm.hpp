#ifndef GREEDYALGORITHM_HPP
#define GREEDYALGORITHM_HPP

#include "Instance.hpp"

class GreedyAlgorithm{
public:
    std::vector<std::vector<int>> nearestNeighbor(const Instance& instance);
    std::vector<std::vector<int>> graspNearestNeighbor(const Instance &instance, double alpha);
};


#endif