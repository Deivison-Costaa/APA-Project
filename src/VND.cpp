    #include "VND.hpp"
    #include <algorithm>
    #include <climits>

    VND::VND(const Instance &instance) : instance(instance) {}

    std::vector<std::vector<int>> VND::execute(std::vector<std::vector<int>> initialSolution)
    {
        currentCost = instance.calculateTotalCost(initialSolution);
        bool improved;

        do
        {
            improved = false;
            // Aplica os movimentos na ordem: swap, move, reinsert
            if (swapFlights(initialSolution))
            {
                improved = true;
                continue;
            }
            if (moveFlight(initialSolution))
            {
                improved = true;
                continue;
            }
            if (reinsertFlight(initialSolution))
            {
                improved = true;
                continue;
            }
        } while (improved);

        return initialSolution;
    }

    // Movimento 1: Troca dois voos consecutivos na mesma pista
    bool VND::swapFlights(std::vector<std::vector<int>> &solution)
    {
        int bestDelta = 0;
        int bestRunway = -1, bestPos = -1;

        for (long unsigned int r = 0; r < solution.size(); ++r) //feito, mas tava dando warning por conta do size()
        {
            for (int i = 0; i < (int)solution[r].size() - 1; ++i)
            {
                // Cria uma cópia da solução para testar o movimento
                auto newSolution = solution; //isso pode ser custoso
                std::swap(newSolution[r][i], newSolution[r][i + 1]);

                // Calcula o delta do custo
                int newCost = instance.calculateTotalCost(newSolution);
                int delta = newCost - currentCost;

                if (delta < bestDelta)
                {
                    bestDelta = delta;
                    bestRunway = r;
                    bestPos = i;
                }
            }
        }

        if (bestDelta < 0)
        {
            std::swap(solution[bestRunway][bestPos], solution[bestRunway][bestPos + 1]);
            currentCost += bestDelta;
            return true;
        }
        return false;
    }

    // Movimento 2: Move um voo para outra pista
    bool VND::moveFlight(std::vector<std::vector<int>> &solution)
    {
        int bestDelta = 0;
        int bestFromRunway = -1, bestToRunway = -1, bestFlightPos = -1;

        for (long unsigned int fromR = 0; fromR < solution.size(); ++fromR)
        {
            for (long unsigned int toR = 0; toR < solution.size(); ++toR)
            {
                if (fromR == toR)
                    continue;
                for (long unsigned int pos = 0; pos < solution[fromR].size(); ++pos)
                {
                    auto newSolution = solution; //novamente, pode ser custoso
                    int flight = newSolution[fromR][pos];
                    newSolution[fromR].erase(newSolution[fromR].begin() + pos);
                    newSolution[toR].push_back(flight);

                    int newCost = instance.calculateTotalCost(newSolution);
                    int delta = newCost - currentCost;

                    if (delta < bestDelta)
                    {
                        bestDelta = delta;
                        bestFromRunway = fromR;
                        bestToRunway = toR;
                        bestFlightPos = pos;
                    }
                }
            }
        }

        if (bestDelta < 0)
        {
            int flight = solution[bestFromRunway][bestFlightPos];
            solution[bestFromRunway].erase(solution[bestFromRunway].begin() + bestFlightPos); //ver custo depois
            solution[bestToRunway].push_back(flight); //coloca no fim
            currentCost += bestDelta;
            return true;
        }
        return false;
    }

    // Movimento 3: Reinsere um voo em outra posição
    bool VND::reinsertFlight(std::vector<std::vector<int>> &solution)
    {
        int bestDelta = 0;
        size_t bestRunway = -1, bestOldPos = -1, bestNewPos = -1;

        for (size_t r = 0; r < solution.size(); ++r)
        {
            for (size_t oldPos = 0; oldPos < solution[r].size(); ++oldPos)
            {
                // Cria uma cópia da solução e remove o voo
                auto newSolution = solution;
                int flight = newSolution[r][oldPos];
                newSolution[r].erase(newSolution[r].begin() + oldPos);

                // Itera até o novo tamanho da pista (após a remoção)
                for (size_t newPos = 0; newPos <= newSolution[r].size(); ++newPos)
                {
                    if (oldPos == newPos)
                        continue;

                    // Cria uma cópia temporária para testar a inserção
                    auto tempSolution = newSolution;
                    tempSolution[r].insert(tempSolution[r].begin() + newPos, flight);

                    // Calcula o delta do custo
                    int newCost = instance.calculateTotalCost(tempSolution);
                    int delta = newCost - currentCost;

                    if (delta < bestDelta)
                    {
                        bestDelta = delta;
                        bestRunway = r;
                        bestOldPos = oldPos;
                        bestNewPos = newPos;
                    }
                }
            }
        }

        if (bestDelta < 0)
        {
            // Aplica o melhor movimento encontrado
            int flight = solution[bestRunway][bestOldPos];
            solution[bestRunway].erase(solution[bestRunway].begin() + bestOldPos);
            solution[bestRunway].insert(solution[bestRunway].begin() + bestNewPos, flight);
            currentCost += bestDelta;
            return true;
        }
        return false;
    }