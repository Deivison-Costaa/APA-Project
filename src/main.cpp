#include "Instance.hpp"
#include "GreedyAlgorithm.hpp"
#include "VariableNeighborhoodDescent.hpp"
#include "MetaHeuristics.hpp"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <vector>
#include <string>
#include <map>

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

// Nova função para testar todas as instâncias do diretório
void testAllInstances()
{
    // Ajuste estes nomes conforme os arquivos na sua pasta "instancias_teste"
    // vector<string> instanceFiles = {
    //     "Instances/n3m10A.txt", "Instances/n3m10B.txt", "Instances/n3m10C.txt", "Instances/n3m10D.txt", "Instances/n3m10E.txt",
    //     "Instances/n3m20A.txt", "Instances/n3m20B.txt", "Instances/n3m20C.txt", "Instances/n3m20D.txt", "Instances/n3m20E.txt",
    //     "Instances/n3m40A.txt", "Instances/n3m40B.txt", "Instances/n3m40C.txt", "Instances/n3m40D.txt", "Instances/n3m40E.txt",
    //     "Instances/n5m50A.txt", "Instances/n5m50B.txt", "Instances/n5m50C.txt", "Instances/n5m50D.txt", "Instances/n5m50E.txt"
    //     };
    vector<string> instanceFiles = {
        "copa_apa/n500m10E.txt"
        // "copa_apa/n700m12E.txt",
        // "copa_apa/n1000m15E.txt",
    };

    map<string, double>
        bestKnown = {
            {"Instances/n3m10A.txt", 7483.0}, // (opt)
            {"Instances/n3m10B.txt", 1277.0}, // (opt)
            {"Instances/n3m10C.txt", 2088.0}, // (opt)
            {"Instances/n3m10D.txt", 322.0},  // (opt)
            {"Instances/n3m10E.txt", 3343.0}, // (opt)
            {"Instances/n3m20A.txt", 3129.0}, // (LB)
            {"Instances/n3m20B.txt", 1258.0}, // (LB)
            {"Instances/n3m20C.txt", 855.0},  // (LB)
            {"Instances/n3m20D.txt", 4357.0}, // (opt)
            {"Instances/n3m20E.txt", 3798.0}, // (opt)
        };

    cout << "\n===== Testando todas as instâncias =====\n"
         << endl;

    for (auto &filePath : instanceFiles)
    {
        // Cria e lê a instância
        Instance instance;
        instance.read(filePath);
        MetaHeuristics meta(instance);

        // Cria uma solução inicial via Greedy (ou outra heurística de sua preferência)
        // GreedyAlgorithm greedy;
        // auto initialSolution = greedy.nearestNeighbor(instance);

        // Mede o tempo do VND
        timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Executa o VND
        // VariableNeighborhoodDescent vnd;
        // auto finalSolution = vnd.vnd(instance, initialSolution);

        auto finalSolution = meta.grasp(10, 0.3);

        // VND vnd(instance);
        // auto finalSolution = vnd.execute(initialSolution);

        clock_gettime(CLOCK_MONOTONIC, &end);
        double timeVND = diffTimespec(start, end);

        // Calcula o custo da solução final
        auto finalCost = instance.calculateTotalCost(finalSolution);

        // Impressão dos resultados
        cout << fixed << setprecision(6);
        cout << "Instância: " << filePath
             << " | Custo (GRASP): " << finalCost;

        // Se existir valor ótimo (ou LB) conhecido para esta instância, calcula o gap
        if (bestKnown.find(filePath) != bestKnown.end())
        {
            cout << fixed << setprecision(6);
            double bestVal = bestKnown[filePath];
            double gap = ((finalCost - bestVal) / bestVal) * 100.0;
            cout << " | Gap: " << gap << "% (Best = " << bestVal << ")";
        }

        instance.flightList = finalSolution;
        instance.printFlightLists();

        cout << " | Tempo: " << timeVND << "s\n";

    }

    cout << "\n=========================================\n"
         << endl;
}

int main(void)
{

    testAllInstances();
    return 0;



    string filePath = "Instances_2/instance4.txt";
    timespec start, end;
    double timeGreedy, timeVND;

    Instance instance;
    instance.read(filePath);

    // Medição do Greedy
    clock_gettime(CLOCK_MONOTONIC, &start);
    GreedyAlgorithm greedy;
    auto test = greedy.nearestNeighbor(instance);
    clock_gettime(CLOCK_MONOTONIC, &end);
    timeGreedy = diffTimespec(start, end);
    auto testCost = instance.calculateTotalCost(test);

    // instance.flightList = test;
    // // instance.printFlightLists();

    // Medição do VND
    clock_gettime(CLOCK_MONOTONIC, &start);
    VariableNeighborhoodDescent vnd;
    auto test2 = vnd.vnd(instance, test);
    clock_gettime(CLOCK_MONOTONIC, &end);
    timeVND = diffTimespec(start, end);
    auto testCost2 = instance.calculateTotalCost(test2);

    // MetaHeuristics meta(instance);
    // clock_gettime(CLOCK_MONOTONIC, &start);
    // auto test3 = meta.grasp(100, 0.2);
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // timeMeta = diffTimespec(start, end);
    // auto testCost3 = instance.calculateTotalCost(test3);

    // instance.flightList = test2;
    // // instance.printFlightLists();

    // // Exibição formatada
    cout << fixed << setprecision(6);
    cout << "Greedy Algorithm: " << testCost
         << " | Tempo: " << timeGreedy << "s\n";

    cout << "VND 1: " << testCost2
         << " | Tempo: " << timeVND << "s\n";

    // cout << "MetaHeuristics: " << testCost3
    //      << " | Tempo: " << timeMeta << "s\n";

    return 0;
}
