#ifndef VND_HPP
#define VND_HPP

#include "Instance.hpp"
#include <vector>

class VND
{
public:
    VND(const Instance &instance);
    std::vector<std::vector<int>> execute(std::vector<std::vector<int>> initialSolution);

private:
    const Instance &instance;
    int currentCost;

    // Movimentos de vizinhança
    bool swapFlights(std::vector<std::vector<int>> &solution);
    bool moveFlight(std::vector<std::vector<int>> &solution);
    bool reinsertFlight(std::vector<std::vector<int>> &solution);
};

#endif