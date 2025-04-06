#include "Instance.hpp"
#include "GreedyAlgorithm.hpp"
#include "VariableNeighborhoodDescent.hpp"
#include <iostream>   // Para saída no console
#include <fstream>    // Para escrita em arquivo
#include <ctime>      // Para medição de tempo
#include <iomanip>    // Para formatação de saída
#include <vector>     // Para lista de instâncias

using namespace std;

// Calcula a diferença entre dois tempos em segundos
double diffTimespec(const timespec &start, const timespec &end) {
    timespec temp;  // Estrutura temporária para o cálculo
    if ((end.tv_nsec - start.tv_nsec) < 0) {  // Se os nanossegundos finais forem menores
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;  // Ajusta os segundos
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;  // Ajusta os nanossegundos
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;  // Diferença direta dos segundos
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;  // Diferença direta dos nanossegundos
    }
    return temp.tv_sec + (temp.tv_nsec * 1e-9);  // Converte para segundos
}

int main() {
    // Lista de arquivos de instâncias a serem testados
    vector<string> files = {"Instances/instance0.txt", "Instances/instance1.txt", 
                            "Instances/instance2.txt", "Instances/instance3.txt"};
    // Abre o arquivo de saída
    ofstream fout("output/output.txt");
    if (!fout) {
        cerr << "Erro ao criar output.txt\n";
        return 1;
    }

    // Para cada arquivo de instância
    for (const auto& filePath : files) {
        Instance instance;
        if (!instance.read(filePath)) {  //
            cerr << "Falha ao ler " << filePath << "\n";
            continue;
        }

        timespec start, end;  // Estruturas para medição de tempo
        cout << "\nProcessando " << filePath << ":\n";  // Exibe a instância atual

        // Medição do algoritmo guloso
        clock_gettime(CLOCK_MONOTONIC, &start);  // Inicia o cronômetro
        GreedyAlgorithm greedy;  // Cria o objeto do algoritmo guloso
        auto greedySol = greedy.nearestNeighbor(instance);  // Executa o guloso
        clock_gettime(CLOCK_MONOTONIC, &end);  // Para o cronômetro
        double timeGreedy = diffTimespec(start, end);  // Calcula o tempo gasto
        int costGreedy = instance.calculateTotalCost(greedySol);  // Calcula o custo da solução

        // Medição do VND
        clock_gettime(CLOCK_MONOTONIC, &start);  // Inicia o cronômetro
        VariableNeighborhoodDescent vnd;  // Cria o objeto do VND
        auto vndSol = vnd.vnd(instance, greedySol);  // Executa o VND a partir da solução gulosa
        clock_gettime(CLOCK_MONOTONIC, &end);  // Para o cronômetro
        double timeVND = diffTimespec(start, end);  // Calcula o tempo gasto
        int costVND = instance.calculateTotalCost(vndSol);  // Calcula o custo da solução

        // Exibe os resultados no console
        cout << fixed << setprecision(6);  // Formata a saída com 6 casas decimais
        cout << "Greedy: " << costGreedy << " | Tempo: " << timeGreedy << "s\n";
        cout << "VND: " << costVND << " | Tempo: " << timeVND << "s\n";

        // Escreve os resultados no arquivo
        fout << "Instância: " << filePath << "\n";  // Cabeçalho da instância
        fout << costVND << "\n";  // Custo total da solução VND
        for (const auto& runway : vndSol) {  // Para cada pista na solução
            for (int flight : runway) {
                fout << flight + 1 << " ";  // Escreve o índice do voo (ajustado para 1-based)
            }
            fout << "\n";
        }
        fout << "\n";

        instance.flightList = vndSol;  // Armazena a solução final na instância
        instance.printFlightLists();   // Exibe a alocação no console
    }

    fout.close();  // Fecha o arquivo de saída
    return 0;      // Termina o programa com sucesso
}
