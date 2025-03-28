#include "VariableNeighborhoodDescent.hpp"
#include "Instance.hpp"
#include <algorithm>
#include <limits>
#include <vector>
#include <tuple>

// Função para explorar a vizinhança de swap dentro da mesma pista
std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::swapWithinRunway(
    const Instance &instance,
    const std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    auto bestNeighbor = solution;
    int bestCost = currentBestCost;

    for (size_t r = 0; r < solution.size(); ++r)
    {
        auto &runway = bestNeighbor[r]; // Trabalha diretamente na cópia
        for (size_t i = 0; i < runway.size(); ++i)
        {
            for (size_t j = i + 1; j < runway.size(); ++j)
            {
                int originalCost = instance.calculateRunwayCost(runway);
                std::swap(runway[i], runway[j]);
                int newCost = instance.calculateRunwayCost(runway);
                int delta = newCost - originalCost;
                
                if ((currentBestCost + delta) < bestCost)
                {
                    bestCost = currentBestCost + delta;
                }
                else
                {
                    std::swap(runway[i], runway[j]); // Reverte
                }
            }
        }
    }
    return {bestNeighbor, bestCost};
}

// Função para explorar a vizinhança de reinserção entre pistas
std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::reinsertBetweenRunways(
    const Instance &instance,
    const std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    auto bestNeighbor = solution;
    int bestCost = currentBestCost;

    for (size_t rSource = 0; rSource < solution.size(); ++rSource)
    {
        const auto &sourceRunway = solution[rSource];
        for (size_t i = 0; i < sourceRunway.size(); ++i)
        {
            int flight = sourceRunway[i];
            auto tempSolution = solution;
            tempSolution[rSource].erase(tempSolution[rSource].begin() + i);
            for (size_t rTarget = 0; rTarget < tempSolution.size(); ++rTarget)
            {
                if (rTarget == rSource)
                    continue;
                for (size_t pos = 0; pos <= tempSolution[rTarget].size(); ++pos)
                {
                    auto newSolution = tempSolution;
                    newSolution[rTarget].insert(newSolution[rTarget].begin() + pos, flight);
                    int newCost = instance.calculateTotalCost(newSolution);
                    if (newCost < bestCost)
                    {
                        bestCost = newCost;
                        bestNeighbor = newSolution;
                    }
                }
            }
        }
    }
    return {bestNeighbor, bestCost};
}

// Função para explorar a vizinhança de troca entre pistas
std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::swapBetweenRunways(
    const Instance &instance,
    const std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    auto bestNeighbor = solution;
    int bestCost = currentBestCost;

    for (size_t r1 = 0; r1 < solution.size(); ++r1)
    {
        for (size_t r2 = r1 + 1; r2 < solution.size(); ++r2)
        {
            for (size_t i = 0; i < solution[r1].size(); ++i)
            {
                for (size_t j = 0; j < solution[r2].size(); ++j)
                {
                    auto tempSolution = solution;
                    std::swap(tempSolution[r1][i], tempSolution[r2][j]);
                    int newCost = instance.calculateTotalCost(tempSolution);
                    if (newCost < bestCost)
                    {
                        bestCost = newCost;
                        bestNeighbor = tempSolution;
                    }
                }
            }
        }
    }
    return {bestNeighbor, bestCost};
}

// Função principal do VND
std::vector<std::vector<int>> VariableNeighborhoodDescent::vnd(
    const Instance &instance,
    const std::vector<std::vector<int>> &initialSolution)
{
    std::vector<std::vector<int>> currentSolution = initialSolution;
    int currentCost = instance.calculateTotalCost(currentSolution);
    bool improved = true;

    while (improved)
    {
        improved = false;
        int neighborhood = 1; // Começa sempre da primeira vizinhança
        while (neighborhood <= 3)
        {
            auto [newSolution, newCost] = [&]() -> std::pair<std::vector<std::vector<int>>, int>
            {
                switch (neighborhood)
                {
                case 1:
                    return swapWithinRunway(instance, currentSolution, currentCost);
                case 2:
                    return reinsertBetweenRunways(instance, currentSolution, currentCost);
                case 3:
                    return swapBetweenRunways(instance, currentSolution, currentCost);
                default:
                    return {currentSolution, currentCost};
                }
            }();

            if (newCost < currentCost)
            {
                currentSolution = newSolution;
                currentCost = newCost;
                improved = true;
                neighborhood = 1; // Reinicia para a primeira vizinhança
            }
            else
            {
                neighborhood++; // Próxima vizinhança
            }
        }
    }
    return currentSolution;
}