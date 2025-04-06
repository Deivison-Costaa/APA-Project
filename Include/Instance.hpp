#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <vector>
#include <string>

// Classe que representa uma instância do problema de escalonamento de voos
class Instance
{
public:
    // Número de voos e pistas na instância
    int numberOfFlights;
    int numberOfRunways;

    // Dados do problema: tempos de liberação (r_i),
    // tempos de processamento (c_i), penalidades (p_i)
    std::vector<int> landingTakeoffTime;  // Tempos de liberação de cada voo
    std::vector<int> processingTime;  // Tempos de processamento (c_i), renomeado de waitingTime
    std::vector<int> penalties;  // Penalidades por atraso de cada voo
    std::vector<std::vector<int>> costMatrix;  // Matriz de tempo de separação (t_{ij})
    

    // Custo total e alocação de voos por pista
    int solution = 0;  // Custo total da solução atual
    std::vector<std::vector<int>> flightList;  // Alocação de voos por pista

    // Lê os dados da instância a partir de um arquivo
    bool read(const std::string &filePath);
    //Exibe os dados da instância no console
    void print() const;
    // Calcula o custo toal de uma solução (soma das penalidades por atraso)
    int calculateTotalCost(const std::vector<std::vector<int>> &schedules) const;
    // Calcula o custo de uma única pista
    int calculateRunwayCost(const std::vector<int> &runway) const;
    // Exibe a alocação de voos por pista
    void printFlightLists() const;
};

#endif
