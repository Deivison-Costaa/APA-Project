#include "VariableNeighborhoodDescent.hpp"
#include "Instance.hpp"
#include <algorithm>
#include <limits>
#include <vector>
#include <tuple>

// Função que troca voos dentro da mesma pista para tentar melhorar o custo
std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::swapWithinRunway(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost; //custo atual
    bool improved = false; //bool pra verificar se houve troca

    for (size_t r = 0; r < solution.size(); ++r) //para cada pista
    {
        auto &runway = solution[r]; //referencia pra facilitar a leitura posterior
        int originalRunwayCost = instance.calculateRunwayCost(runway); //isso aqui vai servir pro cálculo do delta
        //assim evitando ficar chamando ele múltiplas vezes no for posterior

        for (size_t i = 0; i < runway.size(); ++i) //para cada voo
        {
            for (size_t j = i + 1; j < runway.size(); ++j) //para cada outro voo
            //(importante notar que esse for depende do anterior)
            {
                std::swap(runway[i], runway[j]); //troca dois voos
                int newRunwayCost = instance.calculateRunwayCost(runway); //isso aqui é inevitavel, precisa calcular o novo 
                //custo na pista
                int delta = newRunwayCost - originalRunwayCost; //delta (dá pra resumir com a próxima linha, só que assim tá 
                //mais visual)
                int newCost = currentBestCost + delta;

                //vê se a alteração diminuiu o custo
                if (newCost < bestCost)
                {
                    bestCost = newCost;
                    improved = true;
                }
                else
                {
                    std::swap(runway[i], runway[j]); //se não diminuiu volta ao estado anterior
                }
            }
        }
        //Ele retorna assim que uma pista tiver uma melhoria, no caso ele busca exaustivamente em uma pista,
        //caso não melhore vai pra próxima, se melhorar volta
        if (improved)
        {
            currentBestCost = bestCost;
            break;
        }
    }
    return {solution, bestCost};
}

// Função que remove um voo de uma pista e o reinsere em outra para tentar melhorar o custo
std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::reinsertBetweenRunways(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost; ///custo atual
    bool improved = false; // bool pra verificar se houve troca

    for (size_t rSource = 0; rSource < solution.size(); ++rSource) //for da pista de saida
    {
        for (size_t i = 0; i < solution[rSource].size(); ++i) //vê todos os voos da pista de saida
        {
            int flight = solution[rSource][i]; //voo atual
            int originalSourceCost = instance.calculateRunwayCost(solution[rSource]); //custo da pists saida
            solution[rSource].erase(solution[rSource].begin() + i); //remove o voo atual da pista atual
            int costAfterRemoval = instance.calculateRunwayCost(solution[rSource]); // custo da pists saida depois do erase
            int deltaRemoval = costAfterRemoval - originalSourceCost; //delta remoção

            for (size_t rTarget = 0; rTarget < solution.size(); ++rTarget) //for da pista destino
            {
                if (rTarget == rSource) //sem isso ia virar o swapWithimRunway
                    continue;
                
                int originalTargetCost = instance.calculateRunwayCost(solution[rTarget]); //custo da pista destino
                for (size_t pos = 0; pos <= solution[rTarget].size(); ++pos) //voos da pista destino
                {
                    solution[rTarget].insert(solution[rTarget].begin() + pos, flight); //insere o voo removido lá
                    //atrás aqui
                    int costAfterInsertion = instance.calculateRunwayCost(solution[rTarget]); //custo depois da inserção
                    int deltaInsertion = costAfterInsertion - originalTargetCost; //delta inserção
                    int newCost = currentBestCost + deltaRemoval + deltaInsertion; //deltão

                    if (newCost < bestCost) //se deltão diminuiu em relação ao melhor deltão, atualiza
                    {
                        bestCost = newCost;
                        improved = true;
                    }
                    else
                    {
                        solution[rTarget].erase(solution[rTarget].begin() + pos); //se não diminnuiu então desfaz a inserção
                    }
                }
            }
            if (!improved) //se o voo na pista destino não melhorou então adiciona o voo na antiga posição
            {
                solution[rSource].insert(solution[rSource].begin() + i, flight);
            }
            else //caso contrário, atualiza a variavel geral
            {
                currentBestCost = bestCost;
                break;
            }
        }
        if (improved) //se houve uma troca entre pistas que melhorou a solução, retorna (a busca é exaustva dentro de uma dupla
        //de pistas onde a pista origem fornece um voo que verifica exaustivamente todos os voos de todas as outras pistas
        //(exceto a rSource) que melhora a solução)
            break;
    }
    return {solution, bestCost};
}

// Função que troca voos entre duas pistas diferentes para tentar melhorar o custo
std::pair<std::vector<std::vector<int>>, int>
VariableNeighborhoodDescent::swapBetweenRunways(
    const Instance &instance,
    std::vector<std::vector<int>> &solution,
    int currentBestCost)
{
    int bestCost = currentBestCost; //custo atual
    bool improved = false; //bool para verificar se houve trocas

    for (size_t r1 = 0; r1 < solution.size(); ++r1) //para cada pista 
    {
        for (size_t r2 = r1 + 1; r2 < solution.size(); ++r2) //para cada outra pista
        {
            //calcula os custos atuais das duas pistas
            int originalCostR1 = instance.calculateRunwayCost(solution[r1]);
            int originalCostR2 = instance.calculateRunwayCost(solution[r2]);
            for (size_t i = 0; i < solution[r1].size(); ++i) //para cada voo em r1
            {
                for (size_t j = 0; j < solution[r2].size(); ++j) ///para cada voo em r2
                {
                    std::swap(solution[r1][i], solution[r2][j]); //o tanto de vezes que isso troca é uma barbaridade
                    //calcula o novo custo das pistas
                    int newCostR1 = instance.calculateRunwayCost(solution[r1]);
                    int newCostR2 = instance.calculateRunwayCost(solution[r2]);
                    //calculo dos deltas
                    int deltaR1 = newCostR1 - originalCostR1;
                    int deltaR2 = newCostR2 - originalCostR2;
                    int newCost = currentBestCost + deltaR1 + deltaR2;

                    if (newCost < bestCost) //vê se melhorou
                    {
                        bestCost = newCost;
                        improved = true;
                    }
                    else
                    {
                        std::swap(solution[r1][i], solution[r2][j]); //se não melhorou, desfaz
                    }
                }
            }
            if (improved) //atualiza o custo atual se melhorou
            {
                currentBestCost = bestCost;
                break;
            }
        }
        //nesse caso aqui ele roda todos os voos de duas pistas, se melhorou então retorna, do contrario vai testar o próximo
        //par de pistas (os primeiros dois fors)
        if (improved) //se melhorou então sai
             break;
    }
    return {solution, bestCost};
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