#include "MetaHeuristics.hpp"
#include "VariableNeighborhoodDescent.hpp"
#include "GreedyAlgorithm.hpp"
#include <climits>
#include <omp.h>
#include <random>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <numeric>
#include <limits>
#include <utility>
#include <stdexcept>
#include <functional>

// Construtor
MetaHeuristics::MetaHeuristics(const Instance &inst) : instance(inst) {}

// Iterated Local Search (ILS)
std::vector<std::vector<int>> MetaHeuristics::ils(int maxIterations,
                                                  const std::vector<int> &perturbationStrengths,
                                                  std::vector<std::vector<int>> &initialSolution,
                                                  const std::string &outputBaseName)
{
    size_t numThreads = validateNumThreads(perturbationStrengths);
    if (numThreads == 0)
        return {};

    GreedyAlgorithm greedy;
    VariableNeighborhoodDescent vnd;

    auto currentSolution = initialSolution.empty() && instance.numberOfFlights > 0
                               ? greedy.graspNearestNeighbor(instance, 0.2)
                               : initialSolution;
    vnd.vnd(instance, currentSolution);
    auto bestSolution = currentSolution;
    int bestCost = instance.calculateTotalCost(bestSolution);

    std::vector<std::vector<std::vector<int>>> solutionsPerThread(numThreads);
    std::vector<int> costsPerThread(numThreads);
    std::vector<std::mt19937> generators(numThreads);
    std::random_device rd;
    for (size_t i = 0; i < numThreads; ++i)
        generators[i].seed(rd() + i);

    for (int iter = 0; iter < maxIterations; ++iter)
    {
#pragma omp parallel num_threads(numThreads)
        {
            int t = omp_get_thread_num();
            perturb(bestSolution, perturbationStrengths[t], generators[t], solutionsPerThread[t]);
            vnd.vnd(instance, solutionsPerThread[t]);
            costsPerThread[t] = instance.calculateTotalCost(solutionsPerThread[t]);
        }

        int minCost = std::numeric_limits<int>::max();
        int bestThread = 0;
        for (size_t i = 0; i < numThreads; ++i)
        {
            if (costsPerThread[i] < minCost)
            {
                minCost = costsPerThread[i];
                bestThread = i;
            }
        }

        if (minCost < bestCost)
        {
            bestSolution = solutionsPerThread[bestThread];
            bestCost = minCost;
            std::cout << "Iter " << iter
                      << ": novo melhor custo ILS = " << bestCost
                      << " (thread " << bestThread << ")" << std::endl;
            if(minCost < 13877)
                instance.writeFlightList(outputBaseName, solutionsPerThread[bestThread]);
        }else{
            std::cout << "Iter: " << iter << " | best: " << bestCost << std::endl;
        }
    }

    return bestSolution;
}

// Perturba solução
void MetaHeuristics::perturb(const std::vector<std::vector<int>> &solution,
                             int strength,
                             std::mt19937 &gen,
                             std::vector<std::vector<int>> &out)
{
    out = solution;
    int R = instance.numberOfRunways;
    if (R < 2)
        return;

    std::vector<std::function<void(std::vector<std::vector<int>> &, std::mt19937 &)>> perturbations = {
        [this](std::vector<std::vector<int>> &sol, std::mt19937 &g)
        { swapBetweenRunways(sol, g); },
        [this](std::vector<std::vector<int>> &sol, std::mt19937 &g)
        { swapWithinRunway(sol, g); },
        [this](std::vector<std::vector<int>> &sol, std::mt19937 &g)
        { moveWithinRunway(sol, g); },
        [this](std::vector<std::vector<int>> &sol, std::mt19937 &g)
        { removeAndReinsert(sol, g); }};

    std::uniform_int_distribution<int> pertDist(0, perturbations.size() - 1);
    int selectedPert = pertDist(gen);

    for (int i = 0; i < strength; ++i)
    {
        perturbations[selectedPert](out, gen);
    }
}

// Valida número de threads
std::size_t MetaHeuristics::validateNumThreads(const std::vector<int> &strengths) const
{
    if (strengths.empty())
        return 0;
    size_t maxT = static_cast<size_t>(omp_get_max_threads());
    size_t n = std::min(strengths.size(), maxT);
    return n > 0 ? n : 0;
}

// Large Neighborhood Search (LNS)
std::vector<std::vector<int>> MetaHeuristics::lns(int maxIterations,
                                                  int k,
                                                  const std::string &initialPath,
                                                  const std::string &outputBaseName)
{
    GreedyAlgorithm greedy;
    VariableNeighborhoodDescent vnd;

    auto sol = initialPath.empty()
                   ? greedy.nearestNeighbor(instance)
                   : instance.readSolution(initialPath);
    int curCost = instance.calculateTotalCost(sol);
    auto bestSol = sol;
    int bestCost = curCost;

    std::mt19937 gen(std::random_device{}());
    for (int iter = 0; iter < maxIterations; ++iter)
    {
        auto s = sol;
        auto removed = destroy(s, k);
        if (removed.empty() && k > 0)
            continue;
        auto repaired = repair(s, removed);
        vnd.vnd(instance, repaired);
        int c = instance.calculateTotalCost(repaired);
        if (c < curCost)
        {
            sol = repaired;
            curCost = c;
            if (c < bestCost)
            {
                bestSol = sol;
                bestCost = c;
                std::cout << "Iter " << iter
                          << ": novo melhor LNS = " << bestCost << std::endl;
                if(bestCost < 13807) instance.writeFlightList(outputBaseName, bestSol);
            }
        }
        else
        {
            std::cout << "Iter: " << iter
                      << ": | custo: " << c << " | melhor: " << bestCost << std::endl;
        }
    }
    if (!bestSol.empty())
        instance.writeFlightList(outputBaseName, bestSol);
    return bestSol;
}

// Remove k voos
std::vector<int> MetaHeuristics::destroy(std::vector<std::vector<int>> &sol, int k)
{
    std::vector<int> all;
    for (auto &r : sol)
        all.insert(all.end(), r.begin(), r.end());
    if (all.empty() || k <= 0)
        return {};
    k = std::min(k, static_cast<int>(all.size()));
    std::shuffle(all.begin(), all.end(), std::mt19937(std::random_device{}()));
    std::vector<int> rem(all.begin(), all.begin() + k);
    std::vector<bool> mark(instance.numberOfFlights);
    for (int f : rem)
        if (f >= 0 && f < instance.numberOfFlights)
            mark[f] = true;
    for (auto &r : sol)
        r.erase(std::remove_if(r.begin(), r.end(), [&](int f)
                               { return f >= 0 && f < instance.numberOfFlights && mark[f]; }),
                r.end());
    return rem;
}

// Reinsere voos
std::vector<std::vector<int>> MetaHeuristics::repair(const std::vector<std::vector<int>> &part,
                                                     const std::vector<int> &rem)
{
    auto sol = part;
    std::vector<int> cost(sol.size());
    for (size_t i = 0; i < sol.size(); ++i)
        cost[i] = instance.calculateRunwayCost(sol[i]);

    for (int f : rem)
    {
        int bestInc = INT_MAX, br = -1, bp = -1, newC;
        for (size_t r = 0; r < sol.size(); ++r)
        {
            for (size_t p = 0; p <= sol[r].size(); ++p)
            {
                std::vector<int> tmp = sol[r];
                tmp.insert(tmp.begin() + p, f);
                int c = instance.calculateRunwayCost(tmp);
                int inc = c - cost[r];
                if (inc < bestInc)
                {
                    bestInc = inc;
                    br = r;
                    bp = p;
                    newC = c;
                }
            }
        }
        if (br >= 0)
        {
            sol[br].insert(sol[br].begin() + bp, f);
            cost[br] = newC;
        }
    }
    return sol;
}

// Custo com inserção temporária
int MetaHeuristics::calculateCostWithInsertion(std::vector<int> &runway, int flight, size_t pos)
{
    if (pos > runway.size())
        return INT_MAX;
    auto it = runway.insert(runway.begin() + pos, flight);
    int c = instance.calculateRunwayCost(runway);
    runway.erase(it);
    return c;
}

// Troca dentro da mesma pista
void MetaHeuristics::swapWithinRunway(std::vector<std::vector<int>> &sol, std::mt19937 &gen)
{
    int R = instance.numberOfRunways;
    std::uniform_int_distribution<int> dR(0, R - 1);
    int r = dR(gen);
    while (sol[r].size() < 2)
    {
        r = dR(gen);
    }
    std::uniform_int_distribution<int> dP1(0, sol[r].size() - 1);
    std::uniform_int_distribution<int> dP2(0, sol[r].size() - 1);
    int p1, p2;
    do
    {
        p1 = dP1(gen);
        p2 = dP2(gen);
    } while (p1 == p2);
    std::swap(sol[r][p1], sol[r][p2]);
}

// Movimento dentro da mesma pista
void MetaHeuristics::moveWithinRunway(std::vector<std::vector<int>> &sol, std::mt19937 &gen)
{
    int R = instance.numberOfRunways;
    std::uniform_int_distribution<int> dR(0, R - 1);
    int r = dR(gen);
    while (sol[r].empty())
    {
        r = dR(gen);
    }
    std::uniform_int_distribution<int> dP(0, sol[r].size() - 1);
    int from = dP(gen);
    std::uniform_int_distribution<int> dNewP(0, sol[r].size());
    int to = dNewP(gen);
    if (to == from)
        return;
    int flight = sol[r][from];
    sol[r].erase(sol[r].begin() + from);
    if (to > from)
        to--;
    sol[r].insert(sol[r].begin() + to, flight);
}

// Remoção e reinserção
void MetaHeuristics::removeAndReinsert(std::vector<std::vector<int>> &sol, std::mt19937 &gen)
{
    int R = instance.numberOfRunways;
    std::uniform_int_distribution<int> dR(0, R - 1);
    int rSource = dR(gen);
    while (sol[rSource].empty())
    {
        rSource = dR(gen);
    }
    std::uniform_int_distribution<int> dP(0, sol[rSource].size() - 1);
    int from = dP(gen);
    int flight = sol[rSource][from];
    sol[rSource].erase(sol[rSource].begin() + from);

    int rTarget = dR(gen);
    while (rTarget == rSource && R > 1)
    {
        rTarget = dR(gen);
    }
    std::uniform_int_distribution<int> dNewP(0, sol[rTarget].size());
    int to = dNewP(gen);
    sol[rTarget].insert(sol[rTarget].begin() + to, flight);
}

// Swap between runways
void MetaHeuristics::swapBetweenRunways(std::vector<std::vector<int>> &sol, std::mt19937 &gen)
{
    int R = instance.numberOfRunways;
    std::uniform_int_distribution<int> dR(0, R - 1);
    int r1 = dR(gen), r2 = dR(gen);
    int cnt = 0;
    // Tenta selecionar duas pistas diferentes e não vazias, até um limite de tentativas
    while ((r1 == r2 || sol[r1].empty() || sol[r2].empty()) && cnt < R * 2)
    {
        r1 = dR(gen);
        r2 = dR(gen);
        ++cnt;
    }
    // Se não encontrar pistas válidas, retorna sem fazer nada
    if (r1 == r2 || sol[r1].empty() || sol[r2].empty())
        return;
    // Escolhe posições aleatórias nas pistas selecionadas
    std::uniform_int_distribution<int> dP1(0, sol[r1].size() - 1);
    std::uniform_int_distribution<int> dP2(0, sol[r2].size() - 1);
    int p1 = dP1(gen);
    int p2 = dP2(gen);
    // Realiza a troca dos voos
    std::swap(sol[r1][p1], sol[r2][p2]);
}

std::vector<std::vector<int>> MetaHeuristics::lns_parallel(int maxIterations,
                                                           const std::vector<int> &kValues,
                                                           std::vector<std::vector<int>> &initialSolution,
                                                           const std::string &outputBaseName)
{
    size_t numThreads = validateNumThreads(kValues);
    if (numThreads == 0)
        return {};

    GreedyAlgorithm greedy;
    VariableNeighborhoodDescent vnd;

    // Inicializa a solução atual
    auto currentSolution = initialSolution.empty() && instance.numberOfFlights > 0
                               ? greedy.graspNearestNeighbor(instance, 0.2)
                               : initialSolution;
    vnd.vnd(instance, currentSolution);
    auto bestSolution = currentSolution;
    int bestCost = instance.calculateTotalCost(bestSolution);

    // Estruturas para armazenar resultados por thread
    std::vector<std::vector<std::vector<int>>> solutionsPerThread(numThreads);
    std::vector<int> costsPerThread(numThreads);
    std::vector<std::mt19937> generators(numThreads);
    std::random_device rd;
    for (size_t i = 0; i < numThreads; ++i)
        generators[i].seed(rd() + i);

    // Loop principal
    for (int iter = 0; iter < maxIterations; ++iter)
    {
#pragma omp parallel num_threads(numThreads)
        {
            int t = omp_get_thread_num();
            int k = kValues[t % kValues.size()];
            auto s = currentSolution;
            auto removed = destroy(s, k);       // Destruição
            auto repaired = repair(s, removed); // Reparação
            vnd.vnd(instance, repaired);        // Melhoria local
            solutionsPerThread[t] = repaired;
            costsPerThread[t] = instance.calculateTotalCost(repaired);
        }

        // Encontra a melhor solução entre as threads
        int minCost = std::numeric_limits<int>::max();
        int bestThread = 0;
        for (size_t i = 0; i < numThreads; ++i)
        {
            if (costsPerThread[i] < minCost)
            {
                minCost = costsPerThread[i];
                bestThread = i;
            }
        }

        // Atualiza a melhor solução se houver melhoria
        if (minCost < bestCost)
        {
            bestSolution = solutionsPerThread[bestThread];
            bestCost = minCost;
            std::cout << "Iter " << iter
                      << ": novo melhor custo LNS = " << bestCost
                      << " (thread " << bestThread << ")" << std::endl;
            if (minCost < 13807)
                instance.writeFlightList(outputBaseName, solutionsPerThread[bestThread]);
        }
        else
        {
            std::cout << "Iter: " << iter << " | best: " << bestCost << std::endl;
        }
    }

    return bestSolution;
}

// Função repair adaptada com GRASP e RCL
std::vector<std::vector<int>> MetaHeuristics::repair_grasp(const std::vector<std::vector<int>> &part,
                                                           const std::vector<int> &rem,
                                                           double alpha)
{
    auto sol = part;
    std::vector<int> cost(sol.size());
    for (size_t i = 0; i < sol.size(); ++i)
        cost[i] = instance.calculateRunwayCost(sol[i]);

    std::mt19937 gen(std::random_device{}());

    for (int f : rem)
    {
        std::vector<std::tuple<int, int, int>> candidates; // (incremento, pista, posição)
        int minInc = INT_MAX;
        int maxInc = INT_MIN;

        // Calcula o incremento de custo para todas as posições possíveis
        for (size_t r = 0; r < sol.size(); ++r)
        {
            for (size_t p = 0; p <= sol[r].size(); ++p)
            {
                std::vector<int> tmp = sol[r];
                tmp.insert(tmp.begin() + p, f);
                int c = instance.calculateRunwayCost(tmp);
                int inc = c - cost[r];
                candidates.push_back(std::make_tuple(inc, r, p));
                if (inc < minInc)
                    minInc = inc;
                if (inc > maxInc)
                    maxInc = inc;
            }
        }

        // Define o limiar para a RCL
        int threshold = minInc + static_cast<int>(alpha * (maxInc - minInc));

        // Cria a RCL com candidatos cujo incremento <= threshold
        std::vector<std::tuple<int, int, int>> rcl;
        for (auto &cand : candidates)
        {
            if (std::get<0>(cand) <= threshold)
            {
                rcl.push_back(cand);
            }
        }

        // Se a RCL estiver vazia, usa todos os candidatos
        if (rcl.empty())
            rcl = candidates;

        // Seleciona aleatoriamente um candidato da RCL
        std::uniform_int_distribution<int> dist(0, rcl.size() - 1);
        int idx = dist(gen);
        auto [inc, br, bp] = rcl[idx];

        // Insere o voo na posição selecionada
        sol[br].insert(sol[br].begin() + bp, f);
        cost[br] += inc; // Atualiza o custo da pista
    }

    return sol;
}