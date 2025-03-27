#include "Instance.hpp"
#include <fstream>
#include <iostream>

bool Instance::read(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return false;
    }


    file >> numberOfFlights >> numberOfRunways;


    landingTakeoffTime.resize(numberOfFlights);
    waitingTime.resize(numberOfFlights);
    penalties.resize(numberOfFlights);
    costMatrix.resize(numberOfFlights, std::vector<int>(numberOfFlights));


    for (int i = 0; i < numberOfFlights; ++i) 
        file >> landingTakeoffTime[i];

    for (int i = 0; i < numberOfFlights; ++i)
        file >> waitingTime[i];

    for (int i = 0; i < numberOfFlights; ++i)
        file >> penalties[i];

    for (int i = 0; i < numberOfFlights; ++i)
        for (int j = 0; j < numberOfFlights; ++j)
            file >> costMatrix[i][j];


    file.close();
    return true;
}

void Instance::print() const
{
    std::cout << "numberOfFlights: " << numberOfFlights << "";
    std::cout << "\n";
    std::cout << "numberOfRunways: " << numberOfRunways << " ";
    std::cout << "\n";

    std::cout
        << "Landing/Takeoff Times: ";
    for (int val : landingTakeoffTime)
        std::cout << val << " ";
    std::cout << "\n";

    std::cout << "Waiting Times: ";
    for (int val : waitingTime)
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

int Instance::calculateTotalCost(const std::vector<std::vector<int>> &schedules)
{
    int totalCost = 0;

    // Para cada pista no escalonamento
    for (const auto &runwaySchedule : schedules)
    {
        int prevEndTime = 0;
        int prevFlight = -1; // Nenhum voo anterior inicialmente

        // Para cada voo na pista
        for (int flight : runwaySchedule)
        {
            // Tempo de liberação do voo atual
            int ri = landingTakeoffTime[flight];

            // Tempo de espera obrigatório entre voos consecutivos
            int tij = (prevFlight == -1) ? 0 : costMatrix[prevFlight][flight];

            // Calcula o horário de início do voo
            int startTime = std::max(prevEndTime + tij, ri);

            // Atualiza o custo total com a penalidade do atraso
            totalCost += (startTime - ri) * penalties[flight];

            // Atualiza o tempo de término para o próximo voo
            prevEndTime = startTime + waitingTime[flight];
            prevFlight = flight;
        }
    }

    return totalCost;
}