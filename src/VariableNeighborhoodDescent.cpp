#include "VariableNeighborhoodDescent.hpp"
#include "Instance.hpp"
#include <algorithm>
#include <limits>
#include <vector>
#include <tuple>

std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::swapWithinRunway(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;                        // Custo inicial da solução
    std::vector<std::vector<int>> bestSolution = solution; // Cópia da solução inicial para a melhor encontrada
    bool improvementFound = false;                         // Flag para indicar se houve melhoria

    // Itera sobre todas as pistas
    for (size_t r = 0; r < solution.size(); ++r)
    {
        auto &runway = solution[r];                                    // Referência para a pista atual
        int originalRunwayCost = instance.calculateRunwayCost(runway); // Custo original da pista

        // Itera sobre todas as combinações de voos na pista
        for (size_t i = 0; i < runway.size(); ++i)
        {
            for (size_t j = i + 1; j < runway.size(); ++j)
            {
                // Realiza a troca entre runway[i] e runway[j]
                std::swap(runway[i], runway[j]);

                // Calcula o novo custo da pista após a troca
                int newRunwayCost = instance.calculateRunwayCost(runway);
                int delta = newRunwayCost - originalRunwayCost;
                int newCost = currentBestCost + delta;

                // Se o novo custo for menor que o melhor custo encontrado até agora
                if (newCost < bestCost)
                {
                    bestCost = newCost;      // Atualiza o melhor custo
                    bestSolution = solution; // Armazena a solução correspondente
                    improvementFound = true; // Marca que uma melhoria foi encontrada
                }

                // Desfaz a troca para testar a próxima combinação
                std::swap(runway[i], runway[j]);
            }
        }
    }

    // Se uma melhoria foi encontrada, atualiza a solução para a melhor encontrada
    if (improvementFound)
    {
        solution = bestSolution;
    }

    return {solution, bestCost}; // Retorna a solução e o melhor custo
}

// Função que remove um voo de uma pista e o reinsere em outra para tentar melhorar o custo
std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::reinsertBetweenRunways(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;                        // Custo inicial da solução
    std::vector<std::vector<int>> bestSolution = solution; // Cópia da solução inicial para armazenar a melhor encontrada
    bool improvementFound = false;                         // Flag para indicar se houve melhoria

    // Itera sobre todas as pistas de origem
    for (size_t rSource = 0; rSource < solution.size(); ++rSource)
    {
        // Itera sobre todos os voos da pista de origem
        for (size_t i = 0; i < solution[rSource].size(); ++i)
        {
            int flight = solution[rSource][i];                                        // Voo a ser movido
            int originalSourceCost = instance.calculateRunwayCost(solution[rSource]); // Custo da pista de origem antes da remoção
            solution[rSource].erase(solution[rSource].begin() + i);                   // Remove o voo da pista de origem
            int costAfterRemoval = instance.calculateRunwayCost(solution[rSource]);   // Custo após remoção
            int deltaRemoval = costAfterRemoval - originalSourceCost;                 // Diferença de custo devido à remoção

            // Itera sobre todas as pistas de destino
            for (size_t rTarget = 0; rTarget < solution.size(); ++rTarget)
            {
                if (rTarget == rSource) // Evita reinserção na mesma pista
                    continue;

                int originalTargetCost = instance.calculateRunwayCost(solution[rTarget]); // Custo da pista de destino antes da inserção
                // Itera sobre todas as posições possíveis na pista de destino
                for (size_t pos = 0; pos <= solution[rTarget].size(); ++pos)
                {
                    solution[rTarget].insert(solution[rTarget].begin() + pos, flight);        // Insere o voo na posição atual
                    int costAfterInsertion = instance.calculateRunwayCost(solution[rTarget]); // Custo após inserção
                    int deltaInsertion = costAfterInsertion - originalTargetCost;             // Diferença de custo devido à inserção
                    int newCost = currentBestCost + deltaRemoval + deltaInsertion;            // Novo custo total

                    // Se o novo custo for menor que o melhor custo encontrado até agora
                    if (newCost < bestCost)
                    {
                        bestCost = newCost;      // Atualiza o melhor custo
                        bestSolution = solution; // Armazena a solução correspondente
                        improvementFound = true; // Marca que uma melhoria foi encontrada
                    }
                    // Desfaz a inserção para testar a próxima posição
                    solution[rTarget].erase(solution[rTarget].begin() + pos);
                }
            }
            // Restaura o voo na posição original na pista de origem para testar o próximo voo
            solution[rSource].insert(solution[rSource].begin() + i, flight);
        }
    }

    // Se uma melhoria foi encontrada, atualiza a solução para a melhor encontrada
    if (improvementFound)
    {
        solution = bestSolution;
    }

    return {solution, bestCost}; // Retorna a solução e o melhor custo
}

std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::swapBetweenRunways(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost;                        // Custo inicial da solução
    std::vector<std::vector<int>> bestSolution = solution; // Cópia da solução inicial para armazenar a melhor encontrada
    bool improvementFound = false;                         // Flag para indicar se houve melhoria

    // Itera sobre todas as combinações de pistas diferentes
    for (size_t r1 = 0; r1 < solution.size(); ++r1)
    {
        for (size_t r2 = r1 + 1; r2 < solution.size(); ++r2)
        {
            // Calcula os custos originais das duas pistas
            int originalCostR1 = instance.calculateRunwayCost(solution[r1]);
            int originalCostR2 = instance.calculateRunwayCost(solution[r2]);

            // Itera sobre todos os voos da pista r1
            for (size_t i = 0; i < solution[r1].size(); ++i)
            {
                // Itera sobre todos os voos da pista r2
                for (size_t j = 0; j < solution[r2].size(); ++j)
                {
                    // Realiza a troca entre solution[r1][i] e solution[r2][j]
                    std::swap(solution[r1][i], solution[r2][j]);

                    // Calcula os novos custos das pistas após a troca
                    int newCostR1 = instance.calculateRunwayCost(solution[r1]);
                    int newCostR2 = instance.calculateRunwayCost(solution[r2]);

                    // Calcula as diferenças de custo
                    int deltaR1 = newCostR1 - originalCostR1;
                    int deltaR2 = newCostR2 - originalCostR2;
                    int newCost = currentBestCost + deltaR1 + deltaR2;

                    // Se o novo custo for menor que o melhor custo encontrado até agora
                    if (newCost < bestCost)
                    {
                        bestCost = newCost;      // Atualiza o melhor custo
                        bestSolution = solution; // Armazena a solução correspondente
                        improvementFound = true; // Marca que uma melhoria foi encontrada
                    }

                    // Desfaz a troca para testar a próxima combinação
                    std::swap(solution[r1][i], solution[r2][j]);
                }
            }
        }
    }

    // Se uma melhoria foi encontrada, atualiza a solução para a melhor encontrada
    if (improvementFound)
    {
        solution = bestSolution;
    }

    return {solution, bestCost}; // Retorna a solução e o melhor custo
}

std::vector<std::vector<int>> VariableNeighborhoodDescent::vnd(
    const Instance &instance,
    const std::vector<std::vector<int>> &initialSolution)
{
    std::vector<std::vector<int>> currentSolution = initialSolution; //copia a solução inicial
    int currentCost = instance.calculateTotalCost(currentSolution);  //calcula o custo atual
    bool improved = true; //controla o loop principal

    while (improved) //enquanto melhorar
    {
        improved = false;
        int neighborhood = 1; //esse loop segue o que foi ensinado na aula de bruck
        while (neighborhood <= 3)
        {
            // isso aqui é uma lambda function que recebe um parr (ver esse video se tiver dúvidas sobre
            // https://www.youtube.com/watch?v=MH8mLFqj-n8)
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

            //atualmente pensando se vale a pena mudar o bloco anterior pra mexer só por referência, o problema é o risco
            //de vazamentos (aumentou ainda mais após o paralelismo)
            if (newCost < currentCost) //atualiza as variáveis
            {
                currentSolution = newSolution;
                currentCost = newCost;
                improved = true;
                neighborhood = 1;
            }
            else
            {
                neighborhood++; //se não melhorou vai pra próxima vizinhança
            }
        }
    }
    return currentSolution;
}