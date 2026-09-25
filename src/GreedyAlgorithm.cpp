#include "GreedyAlgorithm.hpp"
#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace
{
    std::vector<int> flightsByRelease(const Instance &instance)
    {
        std::vector<int> flights(instance.numberOfFlights);
        std::iota(flights.begin(), flights.end(), 0);
        // Empate na liberação: o voo de maior multa vai primeiro
        std::sort(flights.begin(), flights.end(), [&](int a, int b)
                  {
                      if (instance.landingTakeoffTime[a] != instance.landingTakeoffTime[b])
                          return instance.landingTakeoffTime[a] < instance.landingTakeoffTime[b];
                      if (instance.penalties[a] != instance.penalties[b])
                          return instance.penalties[a] > instance.penalties[b];
                      return a < b; });
        return flights;
    }

    // Horário de início do voo se ele for colocado no fim da pista
    int startTimeAtEnd(const Instance &instance, const std::vector<int> &runway, int runwayEndTime, int flight)
    {
        int tRequired = runway.empty() ? 0 : instance.separation(runway.back(), flight);
        return std::max(instance.landingTakeoffTime[flight], runwayEndTime + tRequired);
    }
}

Schedule GreedyAlgorithm::nearestNeighbor(const Instance &instance) const
{
    const int m = instance.numberOfRunways;
    Schedule runways(m);
    std::vector<int> endTime(m, 0);

    for (int flight : flightsByRelease(instance))
    {
        int bestRunway = 0;
        long long minPenalty = std::numeric_limits<long long>::max();
        int bestStartTime = std::numeric_limits<int>::max();

        for (int r = 0; r < m; ++r)
        {
            int startTime = startTimeAtEnd(instance, runways[r], endTime[r], flight);
            long long penalty = static_cast<long long>(instance.penalties[flight]) *
                                (startTime - instance.landingTakeoffTime[flight]);

            // Empate na multa: prefere a pista que libera o voo mais cedo
            if (penalty < minPenalty || (penalty == minPenalty && startTime < bestStartTime))
            {
                bestRunway = r;
                minPenalty = penalty;
                bestStartTime = startTime;
            }
        }

        runways[bestRunway].push_back(flight);
        endTime[bestRunway] = bestStartTime + instance.waitingTime[flight];
    }

    return runways;
}

Schedule GreedyAlgorithm::graspNearestNeighbor(const Instance &instance, double alpha, std::mt19937 &gen) const
{
    const int m = instance.numberOfRunways;
    Schedule runways(m);
    std::vector<int> endTime(m, 0);
    std::vector<int> startTimes(m);
    std::vector<long long> penalties(m);
    std::vector<int> rcl;
    rcl.reserve(m);

    for (int flight : flightsByRelease(instance))
    {
        for (int r = 0; r < m; ++r)
        {
            startTimes[r] = startTimeAtEnd(instance, runways[r], endTime[r], flight);
            penalties[r] = static_cast<long long>(instance.penalties[flight]) *
                           (startTimes[r] - instance.landingTakeoffTime[flight]);
        }

        auto [minIt, maxIt] = std::minmax_element(penalties.begin(), penalties.end());
        const double threshold = *minIt + alpha * static_cast<double>(*maxIt - *minIt);

        rcl.clear();
        for (int r = 0; r < m; ++r)
            if (penalties[r] <= threshold)
                rcl.push_back(r);

        std::uniform_int_distribution<int> dist(0, static_cast<int>(rcl.size()) - 1);
        const int r = rcl[dist(gen)];

        runways[r].push_back(flight);
        endTime[r] = startTimes[r] + instance.waitingTime[flight];
    }

    return runways;
}
