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
#include <climits>
#include <cstdlib> // Para system()

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
    vector<string> instanceFiles = {
        "Instances/n3m10A.txt", "Instances/n3m10B.txt", "Instances/n3m10C.txt", "Instances/n3m10D.txt", "Instances/n3m10E.txt",
        "Instances/n3m20A.txt", "Instances/n3m20B.txt", "Instances/n3m20C.txt", "Instances/n3m20D.txt", "Instances/n3m20E.txt",
        "Instances/n3m40A.txt", "Instances/n3m40B.txt", "Instances/n3m40C.txt", "Instances/n3m40D.txt", "Instances/n3m40E.txt",
        "Instances/n5m50A.txt", "Instances/n5m50B.txt", "Instances/n5m50C.txt", "Instances/n5m50D.txt", "Instances/n5m50E.txt"
        };
    // vector<string> instanceFiles = {
    //     "copa_apa/n500m10E.txt" //perturbação 3
    //     "copa_apa/n700m12E.txt" //perturbação 4
    //     "copa_apa/n1000m15E.txt" //perturbação 2
    // };

    map<string, double> bestKnown = {
        {"Instances/n3m10A.txt", 7483.0}, // (opt)
        {"Instances/n3m10B.txt", 1277.0}, // (opt)
        {"Instances/n3m10C.txt", 2088.0}, // (opt)
        {"Instances/n3m10D.txt", 322.0},  // (opt)
        {"Instances/n3m10E.txt", 3343.0}, // (opt)
        {"Instances/n3m20A.txt", 8230.0}, // (LB) - Atualizado
        {"Instances/n3m20B.txt", 1820.0}, // (LB) - Atualizado
        {"Instances/n3m20C.txt", 855.0},  // (LB)
        {"Instances/n3m20D.txt", 4357.0}, // (opt)
        {"Instances/n3m20E.txt", 3798.0}, // (opt)
        {"Instances/n3m40A.txt", 112.0},  // (LB) - Adicionado
        {"Instances/n3m40B.txt", 880.0},  // (LB) - Adicionado
        {"Instances/n3m40C.txt", 1962.0}, // (LB) - Adicionado
        {"Instances/n3m40D.txt", 263.0},  // (LB) - Adicionado
        {"Instances/n3m40E.txt", 1192.0}, // (LB) - Adicionado
        {"Instances/n5m50A.txt", 0.0},    // (LB) - Adicionado
        {"Instances/n5m50B.txt", 0.0},    // (LB) - Adicionado
        {"Instances/n5m50C.txt", 0.0},    // (LB) - Adicionado
        {"Instances/n5m50D.txt", 0.0},    // (LB) - Adicionado
        {"Instances/n5m50E.txt", 0.0}     // (LB) - Adicionado
    };

    cout << "\n===== Testando todas as instâncias =====\n\n";


    for (const auto &filePath : instanceFiles)
    {
        int meanCost = 0;
        double meanTime = 0;

        for(int i = 0; i < 10; i++){
            Instance instance;
            if (!instance.read(filePath))
            {
                cerr << "Erro ao ler " << filePath << '\n';
                continue;
            }

            MetaHeuristics meta(instance);

            /* ---------- Greedy + cronômetro ---------- */
            timespec start{}, end{};
            clock_gettime(CLOCK_MONOTONIC, &start);

            auto solution = meta.ils(1000, {1, 2, 3, 4, 5, 6, 7, 8});

            clock_gettime(CLOCK_MONOTONIC, &end);
            const double timeGreedy = diffTimespec(start, end);
            /* ---------------------------------------- */

            const int cost = instance.calculateTotalCost(solution);

            // Gap em relação ao melhor valor conhecido (se existir e for > 0)
            double gap = NAN;
            auto it = bestKnown.find(filePath);
            if (it != bestKnown.end() && it->second > 0.0)
                gap = ((cost - it->second) / it->second) * 100.0;

            /* ---------- Saída no console ---------- */
            cout << fixed << setprecision(6)
                << "Instância: " << filePath
                << " | Custo (Greedy): " << cost;

            if (!std::isnan(gap))
                cout << " | Gap: " << gap << '%';

            cout << " | Tempo: " << timeGreedy << " s\n";
            /* -------------------------------------- */

            /* ---------- Grava solução ---------- */
            // Prefixo para diferenciar cada arquivo de saída
            string outPath = "results/metaheuristics/" + filePath;

            size_t dotPos = outPath.rfind('.');
            if (dotPos != std::string::npos)
                outPath.insert(dotPos, "_" + std::to_string(i));
            
            instance.writeFlightList(outPath, solution);
            /* ----------------------------------- */

            meanCost += cost;
            meanTime += timeGreedy;
        }
    
    cout << "\nmeanCost: " << meanCost / 10 << " meanTime: " << meanTime / 10 << "\n" << endl;
    }

    cout << "\n=========================================\n\n";
}

int main(void)
{

    testAllInstances();
    return 0;



    // string filePath = "Instances_2/instance4.txt";
    // timespec start, end;
    // double timeGreedy, timeVND;

    // Instance instance;
    // instance.read(filePath);

    // // Medição do Greedy
    // clock_gettime(CLOCK_MONOTONIC, &start);
    // GreedyAlgorithm greedy;
    // auto test = greedy.nearestNeighbor(instance);
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // timeGreedy = diffTimespec(start, end);
    // auto testCost = instance.calculateTotalCost(test);

    // // instance.flightList = test;
    // // // instance.printFlightLists();

    // // Medição do VND
    // clock_gettime(CLOCK_MONOTONIC, &start);
    // VariableNeighborhoodDescent vnd;
    // auto test2 = vnd.vnd(instance, test);
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // timeVND = diffTimespec(start, end);
    // auto testCost2 = instance.calculateTotalCost(test2);

    // // MetaHeuristics meta(instance);
    // // clock_gettime(CLOCK_MONOTONIC, &start);
    // // auto test3 = meta.grasp(100, 0.2);
    // // clock_gettime(CLOCK_MONOTONIC, &end);
    // // timeMeta = diffTimespec(start, end);
    // // auto testCost3 = instance.calculateTotalCost(test3);

    // // instance.flightList = test2;
    // // // instance.printFlightLists();

    // // // Exibição formatada
    // cout << fixed << setprecision(6);
    // cout << "Greedy Algorithm: " << testCost
    //      << " | Tempo: " << timeGreedy << "s\n";

    // cout << "VND 1: " << testCost2
    //      << " | Tempo: " << timeVND << "s\n";

    // // cout << "MetaHeuristics: " << testCost3
    // //      << " | Tempo: " << timeMeta << "s\n";

    return 0;
}
