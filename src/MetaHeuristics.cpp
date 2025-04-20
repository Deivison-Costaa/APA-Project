#include "MetaHeuristics.hpp"
#include <climits>
#include <omp.h>
#include <random>
#include <iostream>

MetaHeuristics::MetaHeuristics(const Instance &inst) : instance(inst) {}

std::vector<std::vector<int>> MetaHeuristics::ils(int maxIterations, const std::vector<int> &perturbationStrengths)
{

    size_t numThreads = validateNumThreads(perturbationStrengths);

    GreedyAlgorithm greedy;
    VariableNeighborhoodDescent vnd;

    std::vector<std::vector<int>> currentSolution = greedy.nearestNeighbor(instance);

    vnd.vnd(instance, currentSolution);
    int currentCost = instance.calculateTotalCost(currentSolution);

    std::vector<std::vector<std::vector<int>>> solutions(numThreads);
    std::vector<int> costs(numThreads, INT_MAX);
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int iter = 0; iter < maxIterations; ++iter)
    {

        #pragma omp parallel num_threads(numThreads)
        {
            int threadId = omp_get_thread_num();
            int strength = perturbationStrengths[threadId];

            perturb(currentSolution, strength, gen, solutions[threadId]);

            vnd.vnd(instance, solutions[threadId]);
            costs[threadId] = instance.calculateTotalCost(solutions[threadId]);
        }

        int minCost = INT_MAX;
        int bestIndex = -1;
        for (size_t i = 0; i < numThreads; ++i)
        {
            if (costs[i] < minCost)
            {
                minCost = costs[i];
                bestIndex = i;
            }
        }

        // Atualiza a solução atual se a melhor encontrada for superior
        if (minCost < currentCost)
        {
            currentSolution = solutions[bestIndex];
            currentCost = minCost;
        }
        std::cout << "Iteração: " << iter << " custo: " << minCost << std::endl;
    }

    return currentSolution;
}


void MetaHeuristics::perturb(const std::vector<std::vector<int>> &solution,
                                                      int perturbationStrength,
                                                      std::mt19937 &gen,
                                                      std::vector<std::vector<int>> &perturbedSolution)
{
    perturbedSolution = solution; //custoso, embora o multithread não me deixe escolha
    int numRunways = instance.numberOfRunways;
    std::uniform_int_distribution<int> distRunways(0, numRunways - 1);

    // Realiza trocas aleatórias conforme a força de perturbação
    for (int i = 0; i < perturbationStrength; ++i)
    {
        int r1 = distRunways(gen);
        int r2 = distRunways(gen);
        while (r1 == r2)
        {
            r2 = distRunways(gen); // Garante que as pistas sejam diferentes
        }

        if (!perturbedSolution[r1].empty() && !perturbedSolution[r2].empty())
        {
            std::uniform_int_distribution<int> distPos1(0, perturbedSolution[r1].size() - 1);
            std::uniform_int_distribution<int> distPos2(0, perturbedSolution[r2].size() - 1);

            int pos1 = distPos1(gen);
            int pos2 = distPos2(gen);

            std::swap(perturbedSolution[r1][pos1], perturbedSolution[r2][pos2]);
        }
    }
}

std::size_t MetaHeuristics::validateNumThreads(const std::vector<int> &strengths) const
{
    if (strengths.empty()) // Nenhuma força -> nenhuma thread
        return 0;

    const std::size_t numThreads = std::min(strengths.size(),
                                            static_cast<std::size_t>(omp_get_max_threads()));

    if (numThreads < strengths.size())
        std::cerr << "Aviso: usando " << numThreads
                  << " threads (limite do sistema). "
                  << "Valores após a posição " << numThreads
                  << " não serão usados.\n";

    return numThreads;
}