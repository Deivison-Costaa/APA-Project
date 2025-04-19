#include "MetaHeuristics.hpp"
//#include <cstdlib> // Para rand()
//#include <ctime>   // Para srand()
#include <climits> // Para INT_MAX
#include <omp.h> //Adiciona paralelismo
#include <random> //thread_safee
#include <iostream>

MetaHeuristics::MetaHeuristics(const Instance &inst) 
    //meio que acoplei uma metaheuristica a uma instancia
    //depois discutir se vale a pena desacoplar
    : instance(inst)
{
    //srand(static_cast<unsigned>(time(0))); // Inicializa a semente para números aleatórios (rand() não é thread_safe
    //então foi de base)
}

std::vector<std::vector<int>> MetaHeuristics::ils(int maxIterations, int perturbationStrength)
{
    // Inicialização: Gera solução inicial com Nearest Neighbor
    GreedyAlgorithm greedy;
    std::vector<std::vector<int>> currentSolution = greedy.nearestNeighbor(instance);

    // Aplica busca local (VND) na solução inicial
    VariableNeighborhoodDescent vnd;
    vnd.vnd(instance, currentSolution);
    int currentCost = instance.calculateTotalCost(currentSolution);

    // Mantém a melhor solução encontrada
    std::vector<std::vector<int>> bestSolution = currentSolution;
    int bestCost = currentCost;

    // Gerador de números aleatórios
    std::random_device rd;
    std::mt19937 gen(rd());

    // Loop principal do ILS
    for (int iter = 0; iter < maxIterations; ++iter)
    {
        // Perturbação da solução atual
        std::vector<std::vector<int>> improvedSolution = perturb(currentSolution, perturbationStrength, gen);

        // Busca local na solução perturbada
        vnd.vnd(instance, improvedSolution);
        int improvedCost = instance.calculateTotalCost(improvedSolution);

        // Critério de aceitação: aceita se a nova solução for melhor
        if (improvedCost < currentCost)
        {
            currentSolution = improvedSolution;
            currentCost = improvedCost;

            // Atualiza a melhor solução se necessário (sei que é estranho ter 3 variáveis, mas faz sentido)
            if (improvedCost < bestCost)
            {
                bestCost = improvedCost;
                bestSolution = improvedSolution;
                std::cout << "Solução encontrada: " << improvedCost << std::endl;
            }
        }
    }

    return bestSolution;
}

std::vector<std::vector<int>> MetaHeuristics::perturb(const std::vector<std::vector<int>> &solution,
                                                      int perturbationStrength,
                                                      std::mt19937 &gen)
{
    std::vector<std::vector<int>> perturbedSolution = solution;
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

    return perturbedSolution;
}