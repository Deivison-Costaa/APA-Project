#include "Instance.hpp"
#include <fstream>   // Para leitura de arquivos
#include <iostream>  // Para saída no console

// Lê os dados da instância a partir de um arquivo
bool Instance::read(const std::string &filePath)
{
    // Abertura com verificação do arquivo de entrada
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return false;
    }


    file >> numberOfFlights >> numberOfRunways;  // Lê número de voos e pistas

    // Validação para valores inválidos
    if (numberOfFlights <= 0 || numberOfRunways <= 0) {
        std::cerr << "Invalid number of flights or runways" << std::endl;
        file.close();
        return false;
    }

    // Redimensiona os vetores para o número de voos
    landingTakeoffTime.resize(numberOfFlights); 
    processingTime.resize(numberOfFlights); 
    penalties.resize(numberOfFlights);
    costMatrix.resize(numberOfFlights, std::vector<int>(numberOfFlights));

    //Leitura dos tempos de liberação, processamento, penalidades e matriz de tempos de separação
    for (int i = 0; i < numberOfFlights; ++i) 
        file >> landingTakeoffTime[i];

    for (int i = 0; i < numberOfFlights; ++i)
        file >> processingTime[i];

    for (int i = 0; i < numberOfFlights; ++i)
        file >> penalties[i];

    for (int i = 0; i < numberOfFlights; ++i)
        for (int j = 0; j < numberOfFlights; ++j)
            file >> costMatrix[i][j];

    // Verifica se todos os arquivos foram lidos corretamente
    if (!file.good()) {
        std::cerr << "Incomplete data in file: " << filePath << std::endl;
        file.close();
        return false;
    }


    file.close();
    return true;
}

// Exibe os dados da instância no console
void Instance::print() const
{
    std::cout << "numberOfFlights: " << numberOfFlights << "\n";
    std::cout << "numberOfRunways: " << numberOfRunways << "\n";

    std::cout << "Landing/Takeoff Times: ";
    for (int val : landingTakeoffTime)
        std::cout << val << " ";
    std::cout << "\n";

    std::cout << "Processing Times: ";
    for (int val : processingTime)
        std::cout << val << " ";
    std::cout << "\n";

    std::cout << "Penalties: ";
    for (int val : penalties)
        std::cout << val << " ";
    std::cout << "\n";

    std::cout << "\nCost Matrix:\n";
    for (int i = 0; i < numberOfFlights; ++i)
    {
        for (int j = 0; j < numberOfFlights; ++j)
            std::cout << costMatrix[i][j] << " ";
        std::cout << "\n";
    }
}

// Calcula o custo total de uma solução (soma de todas as penalidades por atraso)
int Instance::calculateTotalCost(const std::vector<std::vector<int>> &schedules) const {
    int totalCost = 0;

    // Para cada pista na solução
    for (const auto &runway : schedules) {
        totalCost += calculateRunwayCost(runway);  // Soma o custo de cada pista
        // Alteração: Refatorado para usar calculateRunwayCost
        // Motivo: Evita duplicação de código (DRY)
    }
    return totalCost;  // Retorna o custo total
}

// Calcula o custo de uma única pista
int Instance::calculateRunwayCost(const std::vector<int> &runway) const
{
    int cost = 0;  // Inicializa o custo da pista
    int prevEndTime = 0;  // Tempo de término do voo anterior
    int prevFlight = -1;  // Índice do voo anterior (-1 se não tiver)

    for (int flight : runway)
    {
        // Tempo de separação entre o voo anterior e o atual (t_{ij})
        int separationTime = (prevFlight == -1) ? 0 : costMatrix[prevFlight][flight];
        // Tempo de início: máximo entre o tempo de liberação e o término anterior + separação
        int startTime = std::max(prevEndTime + separationTime, landingTakeoffTime[flight]);
        // Custo: penalidade pelo atraso (p_i * (s_i - r_i))
        cost += (startTime - landingTakeoffTime[flight]) * penalties[flight];
        prevEndTime = startTime + processingTime[flight];  // Atualiza o tempo de término
        prevFlight = flight;  // Atualiza o voo anterior
    }
    return cost;  // Retorna o custo da pista
}

// Exbie a alocação de voos por pista
void Instance::printFlightLists() const
{
    // Verifica se há uma solução para exibir
    if (flightList.empty()) {
        std::cout << "No solution available yet.\n";
        return;
    }

    std::cout << "Flight Lists:" << std::endl;
    for (size_t i = 0; i < flightList.size(); ++i)
    {
        std::cout << "Runway " << i << ": ";  // Número da pista
        for (int flight : flightList[i])
        {
            std::cout << flight << " ";  // Índice dos voos
        }
        std::cout << std::endl;
    }
    std::cout << "\n\n" << std::endl;
}
