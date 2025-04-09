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

private:
    const Instance &instance;
    // GreedyAlgorithm greedy;
    // VariableNeighborhoodDescent vnd;

    // Método auxiliar para construção gulosa randomizada (GRASP) (não sei se deixo ele aqui ou coloco no greedyAlgorithm
    //discutir depois)
    std::vector<std::vector<int>> randomizedNearestNeighbor(double alpha, std::mt19937 &gen);
};

#endif