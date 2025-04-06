#include "GreedyAlgorithm.hpp"
#include <algorithm>  // Para std::sort e std::max
#include <limits>    //  Para std::numeric_limits 

// Implementa o algoritmo guloso que aloca voos com base na menor penalidade
std::vector<std::vector<int>> GreedyAlgorithm::nearestNeighbor(const Instance &instance)
{
    // Número de voos e pistas, abreviados para facilitar o uso
    int n = instance.numberOfFlights;
    int m = instance.numberOfRunways;

    // Cria um vetor com índices dos voos (0 a n-1) para ordenação
    std::vector<int> flightIndices(n);
    for (int i = 0; i < n; ++i)
    {
        flightIndices[i] = i; // Cada índice representa um voo
    }

    // Ordena os voos por tempo de liberação (r_i) do pouso/decolagem, do menor para o maior
    std::sort(flightIndices.begin(), flightIndices.end(), [&](int a, int b)
              { return instance.landingTakeoffTime[a] < instance.landingTakeoffTime[b]; });

    struct Runway {
        long long lastEndTime = 0;  // Tempo de término do último voo (long long evita overflow)
        std::vector<int> flights;  // Lista de voos alocados nesta pista
    };
    std::vector<Runway> runways(m);  // Vetor com m pistas, inicialmente vazias

    // Para cada voo na ordem de liberação
    for (int idx : flightIndices)
    {
        int bestRunway = -1; // Índice da melhor pista para este voo
        long long minPenalty = std::numeric_limits<long long>::max(); // Menor penalidade encontrada, começa infinita
        long long bestStartTime = 0; // Melhor horário de início encontrado, começa em 0

        // Testa cada pista para encontrar a melhor opção
        for (int r = 0; r < m; ++r)
        {
            // Último voo na pista (ou -1 se vazia)
            int prevFlight = runways[r].flights.empty() ? -1 : runways[r].flights.back();

            // Tempo de separação necessário (t_{ij}) entre o último voo e o atual
            long long tRequired = (prevFlight == -1) ? : instance.costMatrix[prevFlight][idx]; 

            // Tempo de início: máximo entre o tempo de liberação e o término anterior + separação
            long long startTime = std::max(static_cast<long long>(instance.landingTakeoffTime[idx]),
                                            runways[r].lastEndTime + tRequired);

            // Penalidade: p_i * (s_i - r_i), custo de atraso
            long long penalty = static_cast<long long>(instance.penalties[idx]) *
                                (startTime - instance.landingTakeoffTime[idx]);

            // Escolhe a pista com a menor penalidade; em caso de empate, menor tempo de início
            if (penalty < minPenalty || (penalty == minPenalty && startTime < bestStartTime)) {
                bestRunway = r;  // Atualiza a melhor pista
                minPenalty = penalty;  // Atualiza a menor penalidade
                bestStartTime = startTime;  // Atualiza o melhor tempo de início
            }
        }

    // Aloca o voo na melhor pista
    runways[bestRunway].lastEndTime = bestStartTime + instance.processingTime[idx];
    runways[bestRunway].flights.push_back(idx);  // Adiciona o voo à lista da pista
    }

    // Converte o vetor de Runway para o formato de saída (vector vector)
    std::vector<std::vector<int>> result(m);
    for (int i = 0; i < m; ++i) {
        result[i] = std::move(runways[i].flights);  // Usa move para evitar cópia desnecessária
    }
    return result;  // Retorna a solução gulosa
}


// Placeholder para o cheapestInsertion
std::vector<std::vector<int>> GreedyAlgorithm::cheapestInsertion(const Instance& instance) {
    return std::vector<std::vector<int>>();  // Retorna vazio por enquanto
}
