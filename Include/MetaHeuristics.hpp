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

    std::vector<std::vector<int>> ils(int maxIterations, const std::vector<int> &perturbationStrengths);

private:
    const Instance &instance;

    void perturb(
        const std::vector<std::vector<int>> &solution,
        int perturbationStrength,
        std::mt19937 &gen,
        std::vector<std::vector<int>> &perturbedSolution);

    std::size_t validateNumThreads(const std::vector<int> &strengths) const;
};

#endif