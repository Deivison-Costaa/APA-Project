#include "GreedyAlgorithm.hpp"
#include <algorithm>
#include <limits>
#include <vector>

std::vector<std::vector<int>> GreedyAlgorithm::nearestNeighbor(const Instance &instance)
{
    int n = instance.numberOfFlights;
    int m = instance.numberOfRunways; // essa linha e a outra e apenas para diminuir o nome das variáveis

    std::vector<int> flights(n);
    for (int i = 0; i < n; ++i)
    {
        flights[i] = i;
    }

    // Ordena os voos por tempo de liberação do pouso/decolagem (pensei em deixar como parâmetro no instance
    //mas como é chamado basicamente uma vez, então não faz sentido)
    std::sort(flights.begin(), flights.end(), [&](int a, int b)
              { return instance.landingTakeoffTime[a] < instance.landingTakeoffTime[b]; });

    // Cria um vetor de pistas, cada uma com um par (tempo de término do último voo, lista de voos).
    //É possivel usar uma estrutura mais simples pra evitar a conversão no final, mas não afeta tanto o
    //desempenho
    std::vector<std::pair<int, std::vector<int>>> runways(m, {0, {}});

    // Para cada voo
    for (int flight : flights)
    {
        int bestRunway = -1;
        int minPenalty = std::numeric_limits<int>::max();
        int bestStartTime = 0; 

        // Para cada pista
        for (int r = 0; r < m; ++r)
        {
            int prevEndTime = runways[r].first;
            int prevFlight = runways[r].second.empty() ? -1 : runways[r].second.back();
            int tRequired = (prevFlight == -1) ? 0 : instance.costMatrix[prevFlight][flight]; 

            int startTime = std::max(instance.landingTakeoffTime[flight], prevEndTime + tRequired); 
            int penalty = instance.penalties[flight] * (startTime - instance.landingTakeoffTime[flight]);

            if (penalty < minPenalty || (penalty == minPenalty && startTime < bestStartTime))
            {
                bestRunway = r;
                minPenalty = penalty;
                bestStartTime = startTime;
            }
        }

        runways[bestRunway].first = bestStartTime + instance.waitingTime[flight];
        runways[bestRunway].second.push_back(flight);
    }

    //isso aqui vai basicamente pegar a estrutura do runway e 
    //converter pra um vector<vector>>, é um pouco ineficiente fazer isso,
    //no entanto eu considero mais simples ao fazer a alocação dentro da função anterior, fora que a 
    //perda de desempenho no geral é minima
    std::vector<std::vector<int>> result(m);
    for (int i = 0; i < m; ++i)
    {
        result[i] = runways[i].second;
    }
    return result;
}