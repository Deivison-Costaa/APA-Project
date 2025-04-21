#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <vector>
#include <string>

class Instance
{
public:
    //atributos
    int numberOfFlights;
    int numberOfRunways;

    std::vector<int> landingTakeoffTime;
    std::vector<int> waitingTime;
    std::vector<int> penalties;
    std::vector<std::vector<int>> costMatrix;

    bool read(const std::string &filePath);
    void print() const;
    int calculateTotalCost(const std::vector<std::vector<int>> &schedules) const;
    int calculateRunwayCost(const std::vector<int> &runway) const;
    int calculatePartialRunwayCost(const std::vector<int> &runway, int startPos, int prevEndTime, int prevFlight) const;
    std::pair<std::vector<int>, std::vector<int>> calculateRunwayDetails(const std::vector<int> &runway) const;
    void writeFlightList(const std::string &filePath, std::vector<std::vector<int>> flightList) const;
    std::vector<std::vector<int>> readSolution(const std::string &filePath) const;
};

#endif
