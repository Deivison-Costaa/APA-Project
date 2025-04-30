#ifndef METAHEURISTICS_HPP
#define METAHEURISTICS_HPP

#include "Instance.hpp"
#include "GreedyAlgorithm.hpp"
#include "VariableNeighborhoodDescent.hpp"
#include <vector>
#include <random>
#include <string>
#include <cstddef>

class MetaHeuristics
{
public:
    // Construtor
    MetaHeuristics(const Instance &inst);

    // Executa o ILS
    std::vector<std::vector<int>> ils(int maxIterations, const std::vector<int> &perturbationStrengths, std::vector<std::vector<int>> &initialSolution, const std::string &outputBaseName);

    // Executa o LNS
    std::vector<std::vector<int>> lns(int maxIterations, int destructionSize, const std::string &initialSolutionPath, const std::string &outputBaseName);

    std::vector<std::vector<int>> lns_parallel(int maxIterations,
                                                           const std::vector<int> &kValues,
                                                           std::vector<std::vector<int>> &initialSolution,
                                                           const std::string &outputBaseName);

    private : const Instance &instance;

    // Perturba a solução no ILS
    void perturb(const std::vector<std::vector<int>> &solution, int perturbationStrength, std::mt19937 &gen, std::vector<std::vector<int>> &perturbedSolution);

    // Valida o número de threads para o ILS
    std::size_t validateNumThreads(const std::vector<int> &strengths) const;

    // Remove voos no LNS
    std::vector<int> destroy(std::vector<std::vector<int>> &solution, int k);

    // Reinseri voos no LNS
    std::vector<std::vector<int>> repair(const std::vector<std::vector<int>> &partialSolution, const std::vector<int> &removedFlights);

    // Calcula custo com inserção temporária no LNS
    int calculateCostWithInsertion(std::vector<int> &runway, int flight, size_t pos);

    void swapWithinRunway(std::vector<std::vector<int>> &sol, std::mt19937 &gen);
    void moveWithinRunway(std::vector<std::vector<int>> &sol, std::mt19937 &gen);
    void removeAndReinsert(std::vector<std::vector<int>> &sol, std::mt19937 &gen);
    void swapBetweenRunways(std::vector<std::vector<int>> &sol, std::mt19937 &gen);

    std::vector<std::vector<int>> repair_grasp(const std::vector<std::vector<int>> &part,
                                                               const std::vector<int> &rem,
                                                               double alpha);
};


#endif // METAHEURISTICS_HPP