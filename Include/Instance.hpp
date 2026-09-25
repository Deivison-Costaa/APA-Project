#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <vector>
#include <string>
#include <cstddef>

// Uma solução é um vetor de pistas; cada pista é a sequência (0-indexada) dos voos nela.
using Schedule = std::vector<std::vector<int>>;

class Instance
{
public:
    int numberOfFlights = 0;
    int numberOfRunways = 0;

    std::vector<int> landingTakeoffTime; // r_i: horário de liberação do voo i
    std::vector<int> waitingTime;        // c_i: tempo de ocupação da pista pelo voo i
    std::vector<int> penalties;          // p_i: penalidade por unidade de atraso do voo i

    // t_ij armazenada de forma contígua (n*n) para melhor uso de cache
    std::vector<int> separationTimes;

    int separation(int from, int to) const
    {
        return separationTimes[static_cast<std::size_t>(from) * numberOfFlights + to];
    }

    bool read(const std::string &filePath);
    void print() const;

    long long calculateRunwayCost(const std::vector<int> &runway) const;
    long long calculateTotalCost(const Schedule &schedule) const;

    // Verifica se cada voo aparece exatamente uma vez e se o número de pistas está correto
    bool isFeasible(const Schedule &schedule, std::string *reason = nullptr) const;

    // Grava a solução no formato da especificação (custo na 1a linha e voos 1-indexados).
    // Retorna o caminho do arquivo gerado (vazio em caso de erro).
    std::string writeSolution(const std::string &directory, const std::string &instanceName,
                              const Schedule &schedule) const;
    Schedule readSolution(const std::string &filePath) const;
};

#endif
