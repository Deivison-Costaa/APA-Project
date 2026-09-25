#ifndef METAHEURISTICS_HPP
#define METAHEURISTICS_HPP

#include "Instance.hpp"
#include <random>
#include <vector>

struct IlsParameters
{
    int maxIter = 16;        // número de reinícios (construção GRASP + ILS)
    int maxIterIls = -1;     // iterações sem melhora por reinício (-1: 10n se n < 150, senão n/2)
    int maxStrength = 5;     // número máximo de movimentos aleatórios na perturbação
    double timeLimit = 0.0;  // segundos (0: sem limite)
    int threads = 0;         // 0: usa o padrão do OpenMP
    unsigned seed = 0;
    bool verbose = false;
};

struct LnsParameters
{
    int maxIterations = 1000;
    int minDestroy = 10;     // voos removidos por iteração no início
    int maxDestroy = 125;    // ao ultrapassar, reinicia a partir de uma nova solução GRASP
    int destroyStep = 5;     // aumento da destruição após `patience` iterações sem melhora
    int patience = 100;
    double timeLimit = 0.0;
    unsigned seed = 0;
    bool verbose = false;
};

class MetaHeuristics
{
public:
    explicit MetaHeuristics(const Instance &inst);

    // Iterated Local Search com RVND como busca local. Os reinícios são
    // distribuídos entre as threads; cada thread tem seu próprio gerador.
    Schedule ils(const IlsParameters &params, const Schedule &initialSolution = {});

    // Large Neighborhood Search: remove voos aleatórios e os reinsere na
    // posição mais barata, seguido de RVND.
    Schedule lns(const LnsParameters &params, const Schedule &initialSolution = {});

private:
    const Instance &instance;

    void perturb(Schedule &solution, int strength, std::mt19937 &gen) const;

    // Movimentos aleatórios usados na perturbação; retornam false se não se aplicam
    bool doubleBridgeIntra(Schedule &solution, std::mt19937 &gen) const;
    bool swapSegmentsInter(Schedule &solution, std::mt19937 &gen) const;
    bool relocateSegmentInter(Schedule &solution, std::mt19937 &gen) const;

    std::vector<int> destroy(Schedule &solution, int k, std::mt19937 &gen) const;
};

#endif // METAHEURISTICS_HPP
