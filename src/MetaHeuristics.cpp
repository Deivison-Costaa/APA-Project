#include "MetaHeuristics.hpp"
//#include <cstdlib> // Para rand()
//#include <ctime>   // Para srand()
#include <climits> // Para INT_MAX
#include <omp.h> //Adiciona paralelismo
#include <random> //thread_safee

MetaHeuristics::MetaHeuristics(const Instance &inst) 
    //meio que acoplei uma metaheuristica a uma instancia
    //depois discutir se vale a pena desacoplar
    : instance(inst)
{
    //srand(static_cast<unsigned>(time(0))); // Inicializa a semente para números aleatórios (rand() não é thread_safe
    //então foi de base)
}

std::vector<std::vector<int>> MetaHeuristics::grasp(int maxIterations, double alpha)
{
    std::vector<std::vector<int>> globalBestSolution; //melhor solução global
    int globalBestCost = INT_MAX; //melhor custo global

    const int numThreads = omp_get_max_threads(); //pega o número de threads da máquina
    std::vector<int> bestCostPerThread(numThreads, INT_MAX); //cada thread tem sua posição de escrita para custo
    std::vector<std::vector<std::vector<int>>> bestSolPerThread(numThreads); // cada thread tem sua posição de escrita 
    //para solução

#pragma omp parallel for 
//tudo chamado dentro do for após essa diretiva é variavel isolada da thread, o que foi declarado antes é de uso compartilhado
// das threads
    for (int iter = 0; iter < maxIterations; ++iter)
    {
        //verificar qual thread é
        int threadId = omp_get_thread_num();

        std::random_device rd;
        unsigned int seed = rd() ^ (iter << 10) ^ (threadId << 20);
        std::mt19937 gen(seed);

        Instance localInstance = instance; //necessário pela explicação que dei do parallel for

        //VND por thread, se for antes da diretiva da omp ele é compartilhado
        VariableNeighborhoodDescent localVnd;

        // Construção gulosa randomizada baseada em NearestNeighbor
        std::vector<std::vector<int>> initialSolution = randomizedNearestNeighbor(alpha, gen);
        // Busca local com VND
        std::vector<std::vector<int>> improvedSolution = localVnd.vnd(localInstance, initialSolution);
        int improvedCost = localInstance.calculateTotalCost(improvedSolution);

        // Atualiza a melhor solução
        #pragma omp critical //pra evitar race condition, segurança nunca é demais
        if (improvedCost < bestCostPerThread[threadId])
        {
            bestCostPerThread[threadId] = improvedCost;
            bestSolPerThread[threadId] = improvedSolution;
        }
    }

    //Verificar qual é a melhor thread
    for (int t = 0; t < numThreads; ++t)
    {
        if (bestCostPerThread[t] < globalBestCost)
        {
            globalBestCost = bestCostPerThread[t];
            globalBestSolution = bestSolPerThread[t];
        }
    }

    return globalBestSolution;
}

std::vector<std::vector<int>> MetaHeuristics::randomizedNearestNeighbor(double alpha, std::mt19937 &gen) // Passa o gerador por referência
{
    GreedyAlgorithm localGreedy;
    std::vector<std::vector<int>> solution = localGreedy.nearestNeighbor(instance);
    int numRunways = instance.numberOfRunways;
    int numFlights = instance.numberOfFlights;

    // Distribuições para geração de números aleatórios
    std::uniform_real_distribution<double> distAlpha(0.0, 1.0);        // Para a verificação do alpha
    std::uniform_int_distribution<int> distRunways(0, numRunways - 1); // Para seleção de pistas

    for (int i = 0; i < numFlights; ++i)
    {
        // Verifica aleatoriedade usando o gerador (thread-safe)
        if (distAlpha(gen) < alpha)
        {
            // Selecionar pistas aleatórias
            int r1 = distRunways(gen);
            int r2 = distRunways(gen);

            if (!solution[r1].empty() && !solution[r2].empty())
            {
                // Distribuições para posições baseadas no tamanho atual dos vetores
                std::uniform_int_distribution<int> distPos1(0, solution[r1].size() - 1);
                std::uniform_int_distribution<int> distPos2(0, solution[r2].size() - 1);

                // Seleciona posições aleatórias
                int pos1 = distPos1(gen);
                int pos2 = distPos2(gen);

                // Realiza o swap
                std::swap(solution[r1][pos1], solution[r2][pos2]);
            }
        }
    }

    return solution;
}