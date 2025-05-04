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

    //Se rodar atualmente vai testar o ils

    // vector<string> instanceFiles = {
    //     "Instances/n3m10A.txt", "Instances/n3m10B.txt", "Instances/n3m10C.txt", "Instances/n3m10D.txt", "Instances/n3m10E.txt",
    //     "Instances/n3m20A.txt", "Instances/n3m20B.txt", "Instances/n3m20C.txt", "Instances/n3m20D.txt", "Instances/n3m20E.txt",
    //     "Instances/n3m40A.txt", "Instances/n3m40B.txt", "Instances/n3m40C.txt", "Instances/n3m40D.txt", "Instances/n3m40E.txt",
    //     "Instances/n5m50A.txt", "Instances/n5m50B.txt", "Instances/n5m50C.txt", "Instances/n5m50D.txt", "Instances/n5m50E.txt"
    //     };

    vector<string> instanceFiles = {
         "copa_apa/n500m10E.txt"
        //"copa_apa/n700m12E.txt",
        // "copa_apa/n1000m15E.txt"
        };

    // map<string, double>
    //     bestKnown = {
    //         {"Instances/n3m10A.txt", 7483.0}, // (opt)
    //         {"Instances/n3m10B.txt", 1277.0}, // (opt)
    //         {"Instances/n3m10C.txt", 2088.0}, // (opt)
    //         {"Instances/n3m10D.txt", 322.0},  // (opt)
    //         {"Instances/n3m10E.txt", 3343.0}, // (opt)
    //         {"Instances/n3m20A.txt", 8230.0}, // (LB)
    //         {"Instances/n3m20B.txt", 1820.0}, // (LB)
    //         {"Instances/n3m20C.txt", 855.0},  // (LB)
    //         {"Instances/n3m20D.txt", 4357.0}, // (opt)
    //         {"Instances/n3m20E.txt", 3798.0}, // (opt)
    //         {"Instances/n3m40A.txt", 112.0},  // (LB)
    //         {"Instances/n3m40B.txt", 880.0},  // (LB)
    //         {"Instances/n3m40C.txt", 1962.0}, // (LB)
    //         {"Instances/n3m40D.txt", 263.0},  // (LB)
    //         {"Instances/n3m40E.txt", 1192.0}, // (LB)
    //         {"Instances/n5m50A.txt", 0.0},    // (LB)
    //         {"Instances/n5m50B.txt", 0.0},    // (LB)
    //         {"Instances/n5m50C.txt", 0.0},    // (LB)
    //         {"Instances/n5m50D.txt", 0.0},    // (LB)
    //         {"Instances/n5m50E.txt", 0.0}     // (LB)
    //     };

    cout << "\n===== Testando todas as instâncias =====\n\n";


    for (const auto &filePath : instanceFiles)
    {
        Instance instance;
        if (!instance.read(filePath))
        {
            cerr << "Erro ao ler " << filePath << '\n';
            continue;
        }

        MetaHeuristics meta(instance);

        timespec start{}, end{};
        clock_gettime(CLOCK_MONOTONIC, &start);

        // vector<vector<int>> is = instance.readSolution("copa_apa/n700m12E_13877.txt");
        vector<vector<int>> is;

        auto solution = meta.lns(10000000, 10, "", filePath);

        clock_gettime(CLOCK_MONOTONIC, &end);
        const double timeGreedy = diffTimespec(start, end);

        const int cost = instance.calculateTotalCost(solution);

        // // Gap em relação ao melhor valor conhecido (se existir e for > 0)
        // double gap = NAN;
        // auto it = bestKnown.find(filePath);
        // if (it != bestKnown.end() && it->second > 0.0)
        //     gap = ((cost - it->second) / it->second) * 100.0;

        cout << fixed << setprecision(6)
            << "Instância: " << filePath
            << " | Custo (Greedy): " << cost;

        // if (!std::isnan(gap))
        //     cout << " | Gap: " << gap << '%';

        cout << " | Tempo: " << timeGreedy << " s\n";

        string outPath = "Instances/" + filePath;
        
        instance.writeFlightList(filePath, solution);
    }

    cout << "\n=========================================\n\n";
}

int main(void)
{
    testAllInstances();
    return 0;
}
