#ifndef METAHEURISTICS_HPP
#define METAHEURISTICS_HPP

#include "Instance.hpp"
#include "GreedyAlgorithm.hpp"
#include "VariableNeighborhoodDescent.hpp"
#include <vector>
#include <random>

class MetaHeuristics
{
public:

    MetaHeuristics(const Instance &inst);

    std::vector<std::vector<int>> grasp(int maxIterations, double alpha);
    std::vector<std::vector<int>> ils(int maxIterations, int perturbationStrength);

    private : const Instance &instance;

    std::vector<std::vector<int>> randomizedNearestNeighbor(double alpha, std::mt19937 &gen);
    std::vector<std::vector<int>> perturb(
        const std::vector<std::vector<int>> &solution,
        int perturbationStrength,
        std::mt19937 &gen);
};

#endif