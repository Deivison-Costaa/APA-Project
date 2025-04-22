#include "Instance.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <string>

bool Instance::read(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Error opening    //atributos file: " << filePath << std::endl;
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

int Instance::calculateTotalCost(const std::vector<std::vector<int>> &schedules) const
{
    int totalCost = 0;

    // Para cada pista no escalonamento
    for (const auto &runwaySchedule : schedules)
    {
        int prevEndTime = 0;
        int prevFlight = -1;

        for (int flight : runwaySchedule)
        {
            int ri = landingTakeoffTime[flight];
            int tij = (prevFlight == -1) ? 0 : costMatrix[prevFlight][flight];
            int startTime = std::max(prevEndTime + tij, ri);
            totalCost += (startTime - ri) * penalties[flight];
            prevEndTime = startTime + waitingTime[flight];
            prevFlight = flight;
        }
    }

    return totalCost;
}

int Instance::calculateRunwayCost(const std::vector<int> &runway) const
{
    //é parecido com o outro mas pra uma pista só (pras funções de vizinhança calcular só uma pista
    //é bem menos custoso)
    int cost = 0;
    int prevEndTime = 0;
    int prevFlight = -1;
    for (int flight : runway)
    {
        int tij = (prevFlight == -1) ? 0 : costMatrix[prevFlight][flight];
        int startTime = std::max(prevEndTime + tij, landingTakeoffTime[flight]);
        cost += (startTime - landingTakeoffTime[flight]) * penalties[flight];
        prevEndTime = startTime + waitingTime[flight];
        prevFlight = flight;
    }
    return cost;
}

int Instance::calculatePartialRunwayCost(const std::vector<int> &runway, int startPos, int prevEndTime, int prevFlight) const
{
    int cost = 0;
    for (size_t i = startPos; i < runway.size(); ++i)
    {
        int flight = runway[i];
        int tij = (prevFlight == -1) ? 0 : costMatrix[prevFlight][flight];
        int startTime = std::max(prevEndTime + tij, landingTakeoffTime[flight]);
        cost += (startTime - landingTakeoffTime[flight]) * penalties[flight];
        prevEndTime = startTime + waitingTime[flight];
        prevFlight = flight;
    }
    return cost;
}

std::pair<std::vector<int>, std::vector<int>> Instance::calculateRunwayDetails(const std::vector<int> &runway) const
{
    std::vector<int> startTimes(runway.size());
    std::vector<int> accumulatedCosts(runway.size());
    int prevEndTime = 0;
    int prevFlight = -1;

    for (size_t k = 0; k < runway.size(); ++k)
    {
        int flight = runway[k];
        int tij = (prevFlight == -1) ? 0 : costMatrix[prevFlight][flight];
        startTimes[k] = std::max(prevEndTime + tij, landingTakeoffTime[flight]);
        accumulatedCosts[k] = (k > 0 ? accumulatedCosts[k - 1] : 0) +
                              (startTimes[k] - landingTakeoffTime[flight]) * penalties[flight];
        prevEndTime = startTimes[k] + waitingTime[flight];
        prevFlight = flight;
    }
    return {startTimes, accumulatedCosts};
}

void Instance::writeFlightList(const std::string &filePath, std::vector<std::vector<int>> flightList) const
{
    // Cria um path a partir do filePath original
    std::filesystem::path originalPath(filePath);

    //dá pra melhorar isso passando o valor calculado na main aqui, mas esse é o menor dos nossos problemas
    std::string newFileName = originalPath.stem().string() + "_" + std::to_string(calculateTotalCost(flightList)) + originalPath.extension().string();
    std::filesystem::path newFilePath = originalPath.parent_path() / newFileName;

    // Abre (ou cria) o novo arquivo
    std::ofstream outFile(newFilePath);
    if (!outFile.is_open())
    {
        std::cerr << "Error creating file: " << newFilePath << std::endl;
        return;
    }

    outFile << calculateTotalCost(flightList) << "\n";
    
    for (size_t i = 0; i < flightList.size(); ++i)
    {
        for (int flight : flightList[i])
        {
            outFile << flight + 1 << " "; // <- precisa do +1 pra estar de acordo com a especificação do projeto
        }
        outFile << "\n";
    }

    outFile.close();
    std::cout << "Flight list written to: " << newFilePath << std::endl;
}

std::vector<std::vector<int>> Instance::readSolution(const std::string &filePath) const
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir arquivo: " << filePath << std::endl;
        return {};
    }

    std::string line;
    //Descarta a primeira linha
    if (!std::getline(file, line))
        return {}; // arquivo vazio ou sem linhas

    std::vector<std::vector<int>> matrix;
    // Para cada linha restante, extrai os inteiros
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::vector<int> row;
        int value;
        while (iss >> value)
            row.push_back(value - 1);
        if (!row.empty())
            matrix.push_back(std::move(row));
    }

    file.close();
    return matrix;
}