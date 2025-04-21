#include "VariableNeighborhoodDescent.hpp"
#include "Instance.hpp"

#include <vector>
#include <utility>
#include <algorithm> // Para std::sort

std::pair<bool, int> VariableNeighborhoodDescent::swapWithinRunway(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;
    bool improvementFound = false;
    unsigned int bestRunway = 0, bestI = 0, bestJ = 0;

    for (unsigned int r = 0; r < solution.size(); ++r)
    {
        auto &runway = solution[r];
        if (runway.size() < 2)
            continue;

        auto [startTimes, accumulatedCosts] = instance.calculateRunwayDetails(runway);

        for (unsigned int i = 0; i < runway.size(); ++i)
        {
            for (unsigned int j = i + 1; j < runway.size(); ++j)
            {
                int impactPos = std::min(i, j);
                int costBefore = (impactPos > 0) ? accumulatedCosts[impactPos - 1] : 0;

                std::vector<int> newRunway = runway;
                std::swap(newRunway[i], newRunway[j]);

                int prevEndTimeForPartial = (impactPos > 0) ? startTimes[impactPos - 1] + instance.waitingTime[newRunway[impactPos - 1]] : 0;
                int prevFlightForPartial = (impactPos > 0) ? newRunway[impactPos - 1] : -1;
                int costAfter = instance.calculatePartialRunwayCost(newRunway, impactPos, prevEndTimeForPartial, prevFlightForPartial);

                int newCostR = costBefore + costAfter;
                int deltaR = newCostR - accumulatedCosts.back();
                int newCost = currentBestCost + deltaR;

                if (newCost < bestCost)
                {
                    bestCost = newCost;
                    improvementFound = true;
                    bestRunway = r;
                    bestI = i;
                    bestJ = j;
                }
            }
        }
    }

    if (improvementFound)
    {
        std::swap(solution[bestRunway][bestI], solution[bestRunway][bestJ]);
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

    for (unsigned int r1 = 0; r1 < solution.size(); ++r1)
    {
        auto &runway1 = solution[r1];
        auto [startTimes1, accumulatedCosts1] = instance.calculateRunwayDetails(runway1);

        for (unsigned int r2 = r1 + 1; r2 < solution.size(); ++r2)
        {
            auto &runway2 = solution[r2];
            auto [startTimes2, accumulatedCosts2] = instance.calculateRunwayDetails(runway2);

            for (unsigned int i = 0; i < runway1.size(); ++i)
            {
                for (unsigned int j = 0; j < runway2.size(); ++j)
                {
                    int impactPos1 = i;
                    int impactPos2 = j;

                    int costBefore1 = (impactPos1 > 0) ? accumulatedCosts1[impactPos1 - 1] : 0;
                    int costBefore2 = (impactPos2 > 0) ? accumulatedCosts2[impactPos2 - 1] : 0;

                    std::vector<int> newRunway1 = runway1;
                    std::vector<int> newRunway2 = runway2;
                    std::swap(newRunway1[i], newRunway2[j]);

                    int prevEndTimeForPartial1 = (impactPos1 > 0) ? startTimes1[impactPos1 - 1] + instance.waitingTime[newRunway1[impactPos1 - 1]] : 0;
                    int prevFlightForPartial1 = (impactPos1 > 0) ? newRunway1[impactPos1 - 1] : -1;
                    int costAfter1 = instance.calculatePartialRunwayCost(newRunway1, impactPos1, prevEndTimeForPartial1, prevFlightForPartial1);

                    int prevEndTimeForPartial2 = (impactPos2 > 0) ? startTimes2[impactPos2 - 1] + instance.waitingTime[newRunway2[impactPos2 - 1]] : 0;
                    int prevFlightForPartial2 = (impactPos2 > 0) ? newRunway2[impactPos2 - 1] : -1;
                    int costAfter2 = instance.calculatePartialRunwayCost(newRunway2, impactPos2, prevEndTimeForPartial2, prevFlightForPartial2);

                    int newCostR1 = costBefore1 + costAfter1;
                    int newCostR2 = costBefore2 + costAfter2;
                    int deltaR1 = newCostR1 - accumulatedCosts1.back();
                    int deltaR2 = newCostR2 - accumulatedCosts2.back();
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
                }
            }
        }
    }

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
    unsigned int bestR = 0, bestFrom = 0, bestTo = 0;

    for (unsigned int r = 0; r < solution.size(); ++r)
    {
        auto &runway = solution[r];
        if (runway.size() < 2)
            continue;

        auto [startTimes, accumulatedCosts] = instance.calculateRunwayDetails(runway);

        for (unsigned int i = 0; i < runway.size(); ++i)
        {
            int flight = runway[i];
            std::vector<int> tempRunway = runway;
            tempRunway.erase(tempRunway.begin() + i);

            for (unsigned int pos = 0; pos <= tempRunway.size(); ++pos)
            {
                std::vector<int> newRunway = tempRunway;
                newRunway.insert(newRunway.begin() + pos, flight);

                int impactPos = std::min(i, pos);
                int costBefore = (impactPos > 0) ? accumulatedCosts[impactPos - 1] : 0;

                int prevEndTimeForPartial = (impactPos > 0) ? startTimes[impactPos - 1] + instance.waitingTime[newRunway[impactPos - 1]] : 0;
                int prevFlightForPartial = (impactPos > 0) ? newRunway[impactPos - 1] : -1;
                int costAfter = instance.calculatePartialRunwayCost(newRunway, impactPos, prevEndTimeForPartial, prevFlightForPartial);

                int newCostR = costBefore + costAfter;
                int deltaR = newCostR - accumulatedCosts.back();
                int newCost = currentBestCost + deltaR;

                if (newCost < bestCost)
                {
                    bestCost = newCost;
                    improvementFound = true;
                    bestR = r;
                    bestFrom = i;
                    bestTo = pos;
                }
            }
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

std::pair<bool, int> VariableNeighborhoodDescent::reinsertBetweenRunways(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost; // Custo atual da solução
    bool improvementFound = false;  // Indica se houve melhoria
    unsigned int bestRSource = 0, bestI = 0, bestRTarget = 0, bestPos = 0;
    int bestFlight = -1; // Armazena o voo e posições ótimas

    // Iterar sobre todas as pistas de origem
    for (unsigned int rSource = 0; rSource < solution.size(); ++rSource)
    {
        auto &runwaySource = solution[rSource];
        if (runwaySource.empty())
            continue; // Pular pistas vazias

        // Calcular detalhes da pista de origem uma vez
        auto [startTimesSource, accumulatedCostsSource] = instance.calculateRunwayDetails(runwaySource);

        // Testar a remoção de cada voo da pista de origem
        for (unsigned int i = 0; i < runwaySource.size(); ++i)
        {
            int flight = runwaySource[i];
            std::vector<int> tempRunwaySource = runwaySource;
            tempRunwaySource.erase(tempRunwaySource.begin() + i);

            // Calcular custo após remoção usando cálculo parcial
            int costBeforeSource = (i > 0) ? accumulatedCostsSource[i - 1] : 0;
            int prevEndTimeSource = (i > 0) ? startTimesSource[i - 1] + instance.waitingTime[tempRunwaySource[i - 1]] : 0;
            int prevFlightSource = (i > 0) ? tempRunwaySource[i - 1] : -1;
            int costAfterSource = instance.calculatePartialRunwayCost(tempRunwaySource, i, prevEndTimeSource, prevFlightSource);
            int newCostSource = costBeforeSource + costAfterSource;
            int deltaSource = newCostSource - accumulatedCostsSource.back();

            // Iterar sobre todas as pistas de destino
            for (unsigned int rTarget = 0; rTarget < solution.size(); ++rTarget)
            {
                if (rTarget == rSource)
                    continue; // Não inserir na mesma pista
                auto &runwayTarget = solution[rTarget];

                // Calcular detalhes da pista de destino uma vez
                auto [startTimesTarget, accumulatedCostsTarget] = instance.calculateRunwayDetails(runwayTarget);

                // Testar todas as posições de inserção na pista de destino
                for (unsigned int pos = 0; pos <= runwayTarget.size(); ++pos)
                {
                    std::vector<int> newRunwayTarget = runwayTarget;
                    newRunwayTarget.insert(newRunwayTarget.begin() + pos, flight);

                    // Calcular custo após inserção usando cálculo parcial
                    int costBeforeTarget = (pos > 0) ? accumulatedCostsTarget[pos - 1] : 0;
                    int prevEndTimeTarget = (pos > 0) ? startTimesTarget[pos - 1] + instance.waitingTime[newRunwayTarget[pos - 1]] : 0;
                    int prevFlightTarget = (pos > 0) ? newRunwayTarget[pos - 1] : -1;
                    int costAfterTarget = instance.calculatePartialRunwayCost(newRunwayTarget, pos, prevEndTimeTarget, prevFlightTarget);
                    int newCostTarget = costBeforeTarget + costAfterTarget;
                    int deltaTarget = newCostTarget - (runwayTarget.empty() ? 0 : accumulatedCostsTarget.back());

                    // Calcular o novo custo total
                    int newCost = currentBestCost + deltaSource + deltaTarget;

                    // Atualizar se encontrar uma solução melhor
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
                }
            }
        }
    }

    // Aplicar a melhor mudança encontrada
    if (improvementFound)
    {
        solution[bestRSource].erase(solution[bestRSource].begin() + bestI);
        solution[bestRTarget].insert(solution[bestRTarget].begin() + bestPos, bestFlight);
    }

    return {improvementFound, bestCost};
}

void VariableNeighborhoodDescent::vnd(
    const Instance &instance,
    std::vector<std::vector<int>> &initialSolution)
{
    // Custo inicial da solução
    int currentCost = instance.calculateTotalCost(initialSolution);
    bool improved = true;

    // Contadores de sucesso para cada vizinhança (4 vizinhanças, inicializadas em 0)
    std::vector<int> successCounts(4, 0);

    // Enquanto houver melhorias
    while (improved)
    {
        improved = false;

        // Ordem das vizinhanças (1 a 4)
        std::vector<int> neighborhoodOrder = {1, 2, 3, 4};
        // Ordenar com base no sucesso (decrescente)
        std::sort(neighborhoodOrder.begin(), neighborhoodOrder.end(),
                  [&successCounts](int a, int b)
                  {
                      return successCounts[a - 1] > successCounts[b - 1];
                  });

        // Tentar cada vizinhança na ordem definida
        for (int neighborhood : neighborhoodOrder)
        {
            std::pair<bool, int> result; // {houve melhoria?, novo custo}
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
                result = {false, currentCost}; // Caso inválido
                break;
            }

            // Se houve melhoria
            if (result.first)
            {
                currentCost = result.second;       // Atualizar o custo
                improved = true;                   // Marcar que houve melhoria
                successCounts[neighborhood - 1]++; // Incrementar contador de sucesso
                break;                             // Reiniciar com a vizinhança mais bem-sucedida
            }
        }
    }
}