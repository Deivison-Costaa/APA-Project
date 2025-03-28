#include "Instance.hpp"
#include "GreedyAlgorithm.hpp"
#include "VariableNeighborhoodDescent.hpp"
#include "VND.hpp"
#include <iostream>
#include <ctime>
#include <iomanip>

using namespace std;

// Função para calcular diferença entre dois timespec em segundos
double diffTimespec(const timespec &start, const timespec &end)
{
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp.tv_sec + (temp.tv_nsec * 1e-9);
}

int main(void)
{
    string filePath = "Instances/instance3.txt";
    timespec start, end;
    double timeGreedy, timeVND1, timeVND2;

    Instance instance;
    instance.read(filePath);

    // Medição do Greedy
    clock_gettime(CLOCK_MONOTONIC, &start);
    GreedyAlgorithm greedy;
    auto test = greedy.nearestNeighbor(instance);
    clock_gettime(CLOCK_MONOTONIC, &end);
    timeGreedy = diffTimespec(start, end);
    auto testCost = instance.calculateTotalCost(test);

    instance.flightList = test;
    instance.printFlightLists();

    // Medição do VND 1
    clock_gettime(CLOCK_MONOTONIC, &start);
    VariableNeighborhoodDescent vnd;
    auto test2 = vnd.vnd(instance, test);
    clock_gettime(CLOCK_MONOTONIC, &end);
    timeVND1 = diffTimespec(start, end);
    auto testCost2 = instance.calculateTotalCost(test2);

    instance.flightList = test2;
    instance.printFlightLists();

    // Medição do VND 2
    clock_gettime(CLOCK_MONOTONIC, &start);
    VND vnd2(instance);
    auto test3 = vnd2.execute(test);
    clock_gettime(CLOCK_MONOTONIC, &end);
    timeVND2 = diffTimespec(start, end);
    auto testCost3 = instance.calculateTotalCost(test3);

    instance.flightList = test3;
    instance.printFlightLists();

    // Exibição formatada
    cout << fixed << setprecision(6);
    cout << "Greedy Algorithm: " << testCost
         << " | Tempo: " << timeGreedy << "s\n";

    cout << "VND 1: " << testCost2
         << " | Tempo: " << timeVND1 << "s\n";
    cout << "VND 2: " << testCost3
         << " | Tempo: " << timeVND2 << "s\n";

    return 0;
}