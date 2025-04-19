#include "GreedyAlgorithm.hpp"
#include <algorithm>
#include <limits>
#include <vector>
#include <set>
#include <climits>

std::vector<std::vector<int>> GreedyAlgorithm::nearestNeighbor(const Instance &instance)
{
    int n = instance.numberOfFlights;
    int m = instance.numberOfRunways; // essa linha e apenas para diminuir o nome das variáveis

    //aqui estamos dando nome para o vetor de voos
    std::vector<int> flights(n);
    for (int i = 0; i < n; ++i)
    {
        flights[i] = i;
    }

    // Ordena os voos por tempo de liberação do pouso/decolagem
    std::sort(flights.begin(), flights.end(), [&](int a, int b)
              { return instance.landingTakeoffTime[a] < instance.landingTakeoffTime[b]; });

    // Cria um vetor de pistas, cada uma com um par (tempo de término do último voo, lista de voos), depois
    //verificar se há uma forma melhor ou mais eficiente de fazer isso
    std::vector<std::pair<int, std::vector<int>>> runways(m, {0, {}}); // (last_end_time, schedule)

    // Para cada voo
    for (int flight : flights)
    {
        int bestRunway = -1; // Melhor pista para inserir o voo
        int minPenalty = std::numeric_limits<int>::max(); // Menor penalidade encontrada, começa infinita
        int bestStartTime = 0; // Melhor horário de início encontrado, começa em 0

        // Para cada pista
        for (int r = 0; r < m; ++r)
        {
            int prevEndTime = runways[r].first; // Tempo de término do último voo na pista
            int prevFlight = runways[r].second.empty() ? -1 : runways[r].second.back(); // Último voo na pista
            int tRequired = (prevFlight == -1) ? 0 : instance.costMatrix[prevFlight][flight]; // Tempo necessário entre voos

            // Calcula o horário de início do voo entre o horario de liberação do voo e o tempo de término do voo anterior
            //+ o tempo necessário entre os voos
            int startTime = std::max(instance.landingTakeoffTime[flight], prevEndTime + tRequired); 
            // Calcula a penalidade do atraso
            int penalty = instance.penalties[flight] * (startTime - instance.landingTakeoffTime[flight]);

            // Atualiza a melhor pista para inserir o voo. fica entre a pista 
            //com a menor penalidade ou a pista com a menor penalidade e o menor horário de início
            if (penalty < minPenalty || (penalty == minPenalty && startTime < bestStartTime))
            {
                bestRunway = r;
                minPenalty = penalty;
                bestStartTime = startTime;
            }
        }

        // Atualiza o estado da melhor pista
        runways[bestRunway].first = bestStartTime + instance.waitingTime[flight];
        runways[bestRunway].second.push_back(flight);
    }

    //isso aqui vai basicamente pegar a estrutura do runway e 
    //converter pra um vector<vector>>, preciso pensar se tem uma solução melhor
    //provavelmente é melhor fazer isso separado pra livrar essa parte
    std::vector<std::vector<int>> result(m);
    for (int i = 0; i < m; ++i)
    {
        result[i] = runways[i].second;
    }
    return result;
}