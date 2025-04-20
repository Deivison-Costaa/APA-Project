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
    std::vector<int> flightsOrder; 
    std::vector<std::vector<int>> costMatrix;
    

    //Solução e lista de voos, fazem parte da resposta que devemos escrever depois.
    // int solution = 0;
    std::vector<std::vector<int>> flightList;

    bool read(const std::string &filePath);
    void print() const;
    int calculateTotalCost(const std::vector<std::vector<int>> &schedules) const;
    int calculateRunwayCost(const std::vector<int> &runway) const;
    int calculatePartialRunwayCost(const std::vector<int> &runway, int startPos, int prevEndTime, int prevFlight) const;
    void printFlightLists() const;
    void writeFlightList(const std::string &filePath) const;
};

#endif
