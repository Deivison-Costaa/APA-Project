#ifndef VARIABLE_NEIGHBORHOOD_DESCENT_HPP
#define VARIABLE_NEIGHBORHOOD_DESCENT_HPP

#include "Instance.hpp"
#include <random>
#include <vector>

// Busca local com as estruturas de vizinhança:
//   1. swap dentro da pista          2. swap entre pistas
//   3-5. or-opt (bloco de 1, 2 ou 3 voos) dentro da pista
//   6-8. or-opt (bloco de 1, 2 ou 3 voos) entre pistas
//
// Todas usam best improvement. O custo de cada movimento é avaliado sem copiar
// vetores: o prefixo inalterado da pista vem de estruturas auxiliares, e a
// avaliação do sufixo é interrompida quando
//   (a) o custo parcial já não pode melhorar a melhor solução (poda), ou
//   (b) o escalonamento volta a coincidir com o original (sincronização),
//       caso em que o restante do custo é conhecido.
class VariableNeighborhoodDescent
{
public:
    explicit VariableNeighborhoodDescent(const Instance &instance, unsigned seed = 0);

    // VND clássico: vizinhanças em ordem fixa, volta para a primeira a cada melhora.
    long long vnd(Schedule &solution);

    // RVND: vizinhança escolhida aleatoriamente; as que não melhoram são descartadas
    // até que haja uma melhora (quando todas voltam para a lista).
    long long rvnd(Schedule &solution);

    // Insere cada voo de `flights` na posição (pista e índice) de menor aumento de custo.
    void insertCheapest(Schedule &solution, const std::vector<int> &flights);

private:
    enum Neighborhood
    {
        SwapIntra,
        SwapInter,
        OrOpt1Intra,
        OrOpt2Intra,
        OrOpt3Intra,
        OrOpt1Inter,
        OrOpt2Inter,
        OrOpt3Inter,
        NumNeighborhoods
    };

    // Estruturas auxiliares de cada pista
    struct RunwayState
    {
        std::vector<int> start;        // horário de início do voo na posição k
        std::vector<long long> prefix; // custo acumulado das posições 0..k
        long long total = 0;
    };

    // Trecho contíguo de voos usado para montar a sequência virtual de um vizinho
    struct Piece
    {
        const int *flights;
        int length;
    };

    const Instance &instance;
    std::mt19937 gen;

    Schedule *solution = nullptr;
    std::vector<RunwayState> states;
    long long cost = 0;

    void load(Schedule &s);
    void rebuild(int r);
    std::vector<Neighborhood> availableNeighborhoods() const;
    bool explore(Neighborhood n);

    long long prefixCost(int r, int length) const
    {
        return length > 0 ? states[r].prefix[length - 1] : 0;
    }

    // Custo da pista r após trocar as posições [prefixLength, tailStart) pelos trechos
    // `pieces`. Retorna `limit` assim que o custo atingir `limit`.
    long long evaluate(int r, int prefixLength, const Piece *pieces, int numPieces,
                       int tailStart, long long limit) const;

    bool swapIntra();
    bool swapInter();
    bool orOptIntra(int blockSize);
    bool orOptInter(int blockSize);
};

#endif
