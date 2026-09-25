#include "Instance.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <string>

bool Instance::read(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir o arquivo: " << filePath << std::endl;
        return false;
    }

    if (!(file >> numberOfFlights >> numberOfRunways) || numberOfFlights <= 0 || numberOfRunways <= 0)
    {
        std::cerr << "Cabeçalho inválido em: " << filePath << std::endl;
        return false;
    }

    const int n = numberOfFlights;
    landingTakeoffTime.assign(n, 0);
    waitingTime.assign(n, 0);
    penalties.assign(n, 0);
    separationTimes.assign(static_cast<std::size_t>(n) * n, 0);

    for (int i = 0; i < n; ++i)
        file >> landingTakeoffTime[i];
    for (int i = 0; i < n; ++i)
        file >> waitingTime[i];
    for (int i = 0; i < n; ++i)
        file >> penalties[i];
    for (auto &t : separationTimes)
        file >> t;

    if (!file)
    {
        std::cerr << "Arquivo incompleto ou mal formatado: " << filePath << std::endl;
        return false;
    }
    return true;
}

void Instance::print() const
{
    std::cout << "numberOfFlights: " << numberOfFlights << "\n";
    std::cout << "numberOfRunways: " << numberOfRunways << "\n";

    std::cout << "Landing/Takeoff Times: ";
    for (int val : landingTakeoffTime)
        std::cout << val << " ";
    std::cout << "\nWaiting Times: ";
    for (int val : waitingTime)
        std::cout << val << " ";
    std::cout << "\nPenalties: ";
    for (int val : penalties)
        std::cout << val << " ";

    std::cout << "\n\nSeparation Matrix:\n";
    for (int i = 0; i < numberOfFlights; ++i)
    {
        for (int j = 0; j < numberOfFlights; ++j)
            std::cout << separation(i, j) << " ";
        std::cout << "\n";
    }
}

long long Instance::calculateRunwayCost(const std::vector<int> &runway) const
{
    long long cost = 0;
    int prevEndTime = 0;
    int prevFlight = -1;
    for (int flight : runway)
    {
        int tij = (prevFlight == -1) ? 0 : separation(prevFlight, flight);
        int startTime = std::max(prevEndTime + tij, landingTakeoffTime[flight]);
        cost += static_cast<long long>(startTime - landingTakeoffTime[flight]) * penalties[flight];
        prevEndTime = startTime + waitingTime[flight];
        prevFlight = flight;
    }
    return cost;
}

long long Instance::calculateTotalCost(const Schedule &schedule) const
{
    long long totalCost = 0;
    for (const auto &runway : schedule)
        totalCost += calculateRunwayCost(runway);
    return totalCost;
}

bool Instance::isFeasible(const Schedule &schedule, std::string *reason) const
{
    auto fail = [&](const std::string &msg)
    {
        if (reason)
            *reason = msg;
        return false;
    };

    if (static_cast<int>(schedule.size()) != numberOfRunways)
        return fail("número de pistas diferente de " + std::to_string(numberOfRunways));

    std::vector<int> seen(numberOfFlights, 0);
    for (const auto &runway : schedule)
        for (int flight : runway)
        {
            if (flight < 0 || flight >= numberOfFlights)
                return fail("voo fora do intervalo: " + std::to_string(flight + 1));
            if (seen[flight]++)
                return fail("voo repetido: " + std::to_string(flight + 1));
        }

    for (int i = 0; i < numberOfFlights; ++i)
        if (!seen[i])
            return fail("voo não alocado: " + std::to_string(i + 1));
    return true;
}

std::string Instance::writeSolution(const std::string &directory, const std::string &instanceName,
                                    const Schedule &schedule) const
{
    namespace fs = std::filesystem;
    const long long cost = calculateTotalCost(schedule);

    std::error_code ec;
    fs::create_directories(directory, ec);
    fs::path outPath = fs::path(directory) /
                       (fs::path(instanceName).stem().string() + "_" + std::to_string(cost) + ".txt");

    std::ofstream outFile(outPath);
    if (!outFile.is_open())
    {
        std::cerr << "Erro ao criar o arquivo: " << outPath << std::endl;
        return "";
    }

    outFile << cost << "\n";
    for (const auto &runway : schedule)
    {
        for (int flight : runway)
            outFile << flight + 1 << " "; // a especificação usa voos 1-indexados
        outFile << "\n";
    }
    return outPath.string();
}

Schedule Instance::readSolution(const std::string &filePath) const
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir arquivo: " << filePath << std::endl;
        return {};
    }

    std::string line;
    // A primeira linha contém o custo, que é recalculado
    if (!std::getline(file, line))
        return {};

    // Linhas vazias também são pistas (vazias), então não podem ser descartadas.
    Schedule schedule;
    while (std::getline(file, line) && static_cast<int>(schedule.size()) < numberOfRunways)
    {
        std::istringstream iss(line);
        std::vector<int> runway;
        int value;
        while (iss >> value)
            runway.push_back(value - 1);
        schedule.push_back(std::move(runway));
    }
    schedule.resize(numberOfRunways);

    std::string reason;
    if (!isFeasible(schedule, &reason))
    {
        std::cerr << "Solução inválida em " << filePath << ": " << reason << std::endl;
        return {};
    }
    return schedule;
}
