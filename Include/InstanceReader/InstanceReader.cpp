#include "InstanceReader.hpp"
#include <fstream>
#include <iostream>

bool InstanceReader::read(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return false;
    }

    file >> number_of_flights;
    file >> number_of_runways;

    r.resize(number_of_flights);
    c.resize(number_of_flights);
    p.resize(number_of_flights);
    t.resize(number_of_flights, std::vector<int>(number_of_flights));

    for (int i = 0; i < number_of_flights; ++i)
    {
        file >> r[i];
    }

    for (int i = 0; i < number_of_flights; ++i)
    {
        file >> c[i];
    }

    for (int i = 0; i < number_of_flights; ++i)
    {
        file >> p[i];
    }

    for (int i = 0; i < number_of_flights; ++i)
    {
        for (int j = 0; j < number_of_flights; ++j)
        {
            file >> t[i][j];
        }
    }

    file.close();
    return true;
}

void InstanceReader::print() const
{
    std::cout << "Number of flights: " << number_of_flights << "\n";
    std::cout << "Number of runways: " << number_of_runways << "\n\n";

    std::cout << "r: ";
    for (int val : r)
        std::cout << val << " ";
    std::cout << "\nc: ";
    for (int val : c)
        std::cout << val << " ";
    std::cout << "\np: ";
    for (int val : p)
        std::cout << val << " ";
    std::cout << "\n\nMatrix t:\n";
    for (const auto &row : t)
    {
        for (int val : row)
        {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
}
