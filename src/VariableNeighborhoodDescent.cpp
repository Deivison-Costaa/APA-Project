#include "VariableNeighborhoodDescent.hpp"
#include "Instance.hpp"

#include <vector> 
#include <utility>

std::pair<bool, int> VariableNeighborhoodDescent::swapWithinRunway(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;
    bool improvementFound = false;
    unsigned int bestRunway = 0, bestI = 0, bestJ = 0;

    // Itera sobre todas as pistas
    for (unsigned int r = 0; r < solution.size(); ++r)
    {
        auto &runway = solution[r];
        int originalRunwayCost = instance.calculateRunwayCost(runway);

        // Itera sobre todas as combinações de voos na pista
        for (unsigned int i = 0; i < runway.size(); ++i)
        {
            for (unsigned int j = i + 1; j < runway.size(); ++j)
            {
                std::swap(runway[i], runway[j]); // Realiza a troca
                int newRunwayCost = instance.calculateRunwayCost(runway);
                int delta = newRunwayCost - originalRunwayCost;
                int newCost = currentBestCost + delta;

                if (newCost < bestCost)
                {
                    bestCost = newCost;
                    improvementFound = true;
                    bestRunway = r;
                    bestI = i;
                    bestJ = j;
                }
                std::swap(runway[i], runway[j]); // Desfaz a troca
            }
        }
    }

    // Aplica a melhor troca, se encontrada
    if (improvementFound)
    {
        std::swap(solution[bestRunway][bestI], solution[bestRunway][bestJ]);
    }

    return {improvementFound, bestCost};
}

// Função que remove um voo de uma pista e o reinsere em outra para tentar melhorar o custo
std::pair<bool, int> VariableNeighborhoodDescent::reinsertBetweenRunways(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;
    bool improvementFound = false;
    unsigned int bestRSource = 0, bestI = 0, bestRTarget = 0, bestPos = 0;
    int bestFlight = -1;

    // Itera sobre todas as pistas de origem
    for (unsigned int rSource = 0; rSource < solution.size(); ++rSource)
    {
        for (unsigned int i = 0; i < solution[rSource].size(); ++i)
        {
            int flight = solution[rSource][i];
            int originalSourceCost = instance.calculateRunwayCost(solution[rSource]);
            solution[rSource].erase(solution[rSource].begin() + i);
            int costAfterRemoval = instance.calculateRunwayCost(solution[rSource]);
            int deltaRemoval = costAfterRemoval - originalSourceCost;

            // Itera sobre todas as pistas de destino
            for (unsigned int rTarget = 0; rTarget < solution.size(); ++rTarget)
            {
                if (rTarget == rSource)
                    continue;
                int originalTargetCost = instance.calculateRunwayCost(solution[rTarget]);
                for (unsigned int pos = 0; pos <= solution[rTarget].size(); ++pos)
                {
                    solution[rTarget].insert(solution[rTarget].begin() + pos, flight);
                    int costAfterInsertion = instance.calculateRunwayCost(solution[rTarget]);
                    int deltaInsertion = costAfterInsertion - originalTargetCost;
                    int newCost = currentBestCost + deltaRemoval + deltaInsertion;

                    if (newCost < bestCost)
                    {
                        bestCost = newCost;
                        improvementFound = true;
                        bestRSource = rSource;
                        bestI = i;
                        bestRTarget = rTarget;
                        bestPos = pos;
                        bestFlight = flight;
                    }
                    solution[rTarget].erase(solution[rTarget].begin() + pos);
                }
            }
            solution[rSource].insert(solution[rSource].begin() + i, flight);
        }
    }

    // Aplica a melhor reinserção, se encontrada
    if (improvementFound)
    {
        solution[bestRSource].erase(solution[bestRSource].begin() + bestI);
        solution[bestRTarget].insert(solution[bestRTarget].begin() + bestPos, bestFlight);
    }

    return {improvementFound, bestCost};
}

std::pair<bool, int> VariableNeighborhoodDescent::swapBetweenRunways(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;
    bool improvementFound = false;
    unsigned int bestR1 = 0, bestI = 0, bestR2 = 0, bestJ = 0;

    // Itera sobre todas as combinações de pistas diferentes
    for (unsigned int r1 = 0; r1 < solution.size(); ++r1)
    {
        for (unsigned int r2 = r1 + 1; r2 < solution.size(); ++r2)
        {
            int originalCostR1 = instance.calculateRunwayCost(solution[r1]);
            int originalCostR2 = instance.calculateRunwayCost(solution[r2]);

            for (unsigned int i = 0; i < solution[r1].size(); ++i)
            {
                for (unsigned int j = 0; j < solution[r2].size(); ++j)
                {
                    std::swap(solution[r1][i], solution[r2][j]);
                    int newCostR1 = instance.calculateRunwayCost(solution[r1]);
                    int newCostR2 = instance.calculateRunwayCost(solution[r2]);
                    int deltaR1 = newCostR1 - originalCostR1;
                    int deltaR2 = newCostR2 - originalCostR2;
                    int newCost = currentBestCost + deltaR1 + deltaR2;

                    if (newCost < bestCost)
                    {
                        bestCost = newCost;
                        improvementFound = true;
                        bestR1 = r1;
                        bestI = i;
                        bestR2 = r2;
                        bestJ = j;
                    }
                    std::swap(solution[r1][i], solution[r2][j]); // Desfaz a troca
                }
            }
        }
    }

    // Aplica a melhor troca, se encontrada
    if (improvementFound)
    {
        std::swap(solution[bestR1][bestI], solution[bestR2][bestJ]);
    }

    return {improvementFound, bestCost};
}

std::pair<bool, int> VariableNeighborhoodDescent::reinsertWithinRunway(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;
    bool improvementFound = false;
    unsigned bestR = 0, bestFrom = 0, bestTo = 0;

    for (unsigned r = 0; r < solution.size(); ++r)
    {
        auto &runway = solution[r];
        int originalCost = instance.calculateRunwayCost(runway);

        for (unsigned i = 0; i < runway.size(); ++i)
        {
            int flight = runway[i];
            runway.erase(runway.begin() + i); // remove

            for (unsigned pos = 0; pos <= runway.size(); ++pos)
            {
                runway.insert(runway.begin() + pos, flight); // insere
                int newCost = currentBestCost - originalCost + instance.calculateRunwayCost(runway);

                if (newCost < bestCost)
                {
                    bestCost = newCost;
                    improvementFound = true;
                    bestR = r;
                    bestFrom = i;
                    bestTo = pos;
                }
                runway.erase(runway.begin() + pos); // desfaz
            }
            runway.insert(runway.begin() + i, flight); // restaura
        }
    }

    if (improvementFound)
    {
        int flight = solution[bestR][bestFrom];
        solution[bestR].erase(solution[bestR].begin() + bestFrom);
        solution[bestR].insert(solution[bestR].begin() + bestTo, flight);
    }
    return {improvementFound, bestCost};
}

void VariableNeighborhoodDescent::vnd(
    const Instance &instance,
    std::vector<std::vector<int>> &initialSolution)
{
    int currentCost = instance.calculateTotalCost(initialSolution);
    bool improved = true;

    while (improved)
    {
        improved = false;
        int neighborhood = 1;
        while (neighborhood <= 4)
        {
            std::pair<bool, int> result;
            switch (neighborhood)
            {
            case 1:
                result = swapWithinRunway(instance, initialSolution, currentCost);
                break;
            case 2:
                result = swapBetweenRunways(instance, initialSolution, currentCost);
                break;
            case 3:
                result = reinsertWithinRunway(instance, initialSolution, currentCost);
                break;
            case 4:
                result = reinsertBetweenRunways(instance, initialSolution, currentCost);
                break;
            default:
                result = {false, currentCost};
                break;
            }

            if (result.first) // Se houve melhoria
            {
                currentCost = result.second;
                improved = true;
                neighborhood = 1; // Reinicia para a primeira vizinhança
            }
            else
            {
                neighborhood++; // Próxima vizinhança
            }
        }
    }
}