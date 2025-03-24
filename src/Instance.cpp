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
    {
        file >> landingTakeoffTime[i];
    }


    for (int i = 0; i < numberOfFlights; ++i)
    {
        file >> waitingTime[i];
    }


    for (int i = 0; i < numberOfFlights; ++i)
    {
        file >> penalties[i];
    }

    // Lê matriz de custo
    for (int i = 0; i < numberOfFlights; ++i)
    {
        for (int j = 0; j < numberOfFlights; ++j)
        {
            file >> costMatrix[i][j];
        }
    }

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
        {
            std::cout << costMatrix[i][j] << " ";
        }
        std::cout << "\n";
    }
}